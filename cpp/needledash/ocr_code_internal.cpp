#include <iostream>     // for test
#include <cmath>
#include <tuple>

#include "hardcode.hpp"
#include "ocr_code_internal.hpp"
#include "box_info_impl.hpp"

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>
#include "Excalibur/operation_make_border.hpp"

#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "Primitives/logger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <fstream>
#include <sstream>
#include <cmath>

#include <abi/param_vector.hpp>
#include <utility>

#define PI 3.14159265358979323846


namespace glasssix::needledash
{
    class ocr_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
                : model_directory_{ std::string(model_directory) }, device_{ device }
        {
            meter_sim_instance_ = std::make_unique<excalibur::pipeline<float>>(get_model_params("meter_sim", false), std::string(model_directory) + "/" + "meter_sim" + ".racy", device);
        }

        exposing::param_vector<needledash::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int type, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            // cut roi image to detect
            cv::Mat roi_image;
            cv::Rect roi(roi_x, roi_y, roi_width, roi_height);
            image(roi).copyTo(roi_image);

            auto result = run_detect(roi_image, type, param_map);

            needledash::box_info_internal result_internal;

            result_internal.x1 = std::get<1>(result).x;
            result_internal.y1 = std::get<1>(result).y;
            result_internal.x2 = std::get<2>(result).x;
            result_internal.y2 = std::get<2>(result).y;

            result_internal.strinfo = glasssix::exposing::param_string(std::get<0>(result));

            auto results = exposing::make_param_vector<needledash::box_info>();

            results.push_back(glasssix::exposing::make_as_first<box_info_impl>(result_internal));

            return results;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        struct point {
            float x1;
            float y1;
            float x2;
            float y2;
        };

        struct pt {
            float x;
            float y;
            float w;
            float h;
            float score;
            int category;
        };

        struct keypt {
            float x;
            float y;
            float score;
        };

        // sigmoid
        static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        static std::pair<cv::Mat, float> letterbox(cv::Mat& img) {
            auto new_shape = cv::Size(640, 640);
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)new_shape.width;
            float ratio_h = (float)H / (float)new_shape.height;
            float ratio = ratio_w;

            cv::Mat resize_img;
            if (H == new_shape.height && W == new_shape.width) {
                resize_img = img;
            }
            else {
                if (ratio_w == ratio_h) {
                    cv::resize(img, resize_img, cv::Size2i{ new_shape.width, new_shape.height });
                }
                else if (ratio_w > ratio_h) {

                    int new_x = new_shape.width;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((new_shape.height - new_y) / 2);
                    int pad2 = new_shape.height - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, pad1 + pad2, 0, 0, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });
                }
                else {
                    ratio = ratio_h;
                    int new_y = new_shape.height;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((new_shape.width - new_x) / 2);
                    int pad2 = new_shape.width - new_x - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, pad1 + pad2, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });
                }
            }

            return { resize_img, ratio };
        }

        /**
         * @brief preprocess
         * 
         */
        std::pair<cv::Mat, float> preprocess(cv::Mat& image) {
            // letterbox
            cv::Mat crop_image;
            float ratio;
            std::tie(crop_image, ratio) = letterbox(image);

            // cvt BGR2RGB
            cv::Mat rgb_image;
            cv::cvtColor(crop_image, rgb_image, cv::COLOR_BGR2RGB);

            return std::make_pair(rgb_image, ratio);
        }

        /**
         * @brief findmax
         */
        int findMax(float num1, float num2, float num3) {
            if (num1 >= num2 && num1 >= num3) {
                return 0;
            }
            else if (num2 >= num1 && num2 >= num3) {
                return 1;
            }
            else {
                return 2;
            }
        }

        /**
         * @brief concat
         * @param outs      : excalibur inference output
         * @param conf_thres
         * @return  pt_location, key_location
         */
        std::pair<std::vector<pt>, std::vector<keypt>> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs, float conf_thres)
        {
            const float anchors[4][6] = { {19,27,    44,40,    38,94},
                                          {96,68,    86,152,   180,137},
                                          {140,301,  303,264,  238,542},
                                          {436,615,  739,380,  925,792} };
										  
            const float stride[4] = {8.0, 16.0, 32.0};

            std::vector<pt> pt_location;
            std::vector<keypt> key_location;
            auto class_pred = [](float x, float y) {if (x > y) return 1; else return 0; };

            for (int n = 0; n < 3; n++)
            {
                int num_grid_x = (int)(640 / stride[n]);
                int num_grid_y = (int)(640 / stride[n]);

                int ind = 0;
                const float* ptr_out = outs[n]->cpu_data();
                // channel
                for (int q = 0; q < 3; q++)
                {
                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for (int i = 0; i < num_grid_x; i++)
                    {
                        for (int j = 0; j < num_grid_y; j++)
                        {
                            const float* pdata = ptr_out + ind * 23;

                            float box_score = sigmoid_x(pdata[4]);

                            if (box_score >= conf_thres)
                            {
                                pt pt_temp{};
                                pt_temp.x = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                                pt_temp.y = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                                pt_temp.w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                                pt_temp.h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h
                                pt_temp.score = sigmoid_x(pdata[4]);

                                pt_temp.category = findMax(sigmoid_x(pdata[5]), sigmoid_x(pdata[6]), sigmoid_x(pdata[7]));

                                pt_location.push_back(pt_temp);

                                //key-points
                                for (int group = 0; group < 5; ++group) {
                                    keypt ky_temp{};
                                    ky_temp.x = (pdata[8 + group * 3] * 2.f - 0.5f + j) * stride[n]; //point x
                                    ky_temp.y = (pdata[9 + group * 3] * 2.f - 0.5f + i) * stride[n]; //point y
                                    ky_temp.score = sigmoid_x(pdata[10 + group * 3]); //point score

                                    key_location.push_back(ky_temp);
                                }

                            }

                            ind++;
                        }
                    }
                }
            }

            return std::make_pair(pt_location, key_location);
        }

        /**
         * @fun computNmsInput
         * @param src, max_wh
         * @return bboxes, scores, category
         * @details slice src into bboxes and confidence, which need by dnn::NMS
         */
        static std::tuple<std::vector<cv::Rect>, std::vector<float>, std::vector<int>> computeNmsInput(std::vector<pt>& src)
        {
            std::vector<cv::Rect> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for (auto const& it : src)
            {
                cv::Rect temp;
                temp.x = static_cast<int>(it.x);
                temp.y = static_cast<int>(it.y);
                temp.width = static_cast<int>(it.w);
                temp.height = static_cast<int>(it.h);
                boxes.push_back(temp);
                scores.push_back(it.score);
                category.push_back(it.category);
            }
            return std::make_tuple(boxes, scores, category);
        }

        /**
         * @fun non_max_suppression
         * @param pt_location, key_location, conf_thres, iou_thres
         * @return pt_nms, key_nms
         * @details non_max_suppression
         */
        std::pair<std::vector<point>, std::vector<keypt>> non_max_suppression(std::vector<pt>& pt_location, std::vector<keypt>& key_location, float conf_thres, float iou_thres)
        {
            std::vector<point> pt_nms;
            std::vector<keypt> key_nms;

            std::vector<cv::Rect> boxes;
            std::vector<float> scores;
            std::vector<int> category;

            std::tie(boxes, scores, category) = computeNmsInput(pt_location);

            std::vector<int> indices;
            cv::dnn::NMSBoxes(boxes, scores, conf_thres, iou_thres, indices, 1.f, 1);

            for (auto const& it : indices)
            {
                point pt_temp{};

                pt_temp.x1 = pt_location[it].x - pt_location[it].w / 2;
                pt_temp.y1 = pt_location[it].y - pt_location[it].h / 2;

                pt_temp.x2 = pt_location[it].x + pt_location[it].w / 2;
                pt_temp.y2 = pt_location[it].y + pt_location[it].h / 2;

                pt_nms.push_back(pt_temp);

                for (int j = 0; j < 5; j++)
                {
                    key_nms.push_back(key_location[it * 5 + j]);
                }
            }

            return std::make_pair(pt_nms, key_nms);
        }


        /**
         * @fun scale_coords on keypt
         * @param coords, old_image, new_image, step
         * @return coords
         */
        std::vector<cv::Point2f> scale_coords_keypt(std::vector<keypt>& coords, cv::Size& old_shape, cv::Size& new_shape)
        {
            std::vector<cv::Point2f> scale_coords_keypt;

            auto gain = std::min((float)new_shape.width / (float)old_shape.width, (float)new_shape.height / (float)old_shape.height);

            auto pad = std::make_pair((new_shape.width - old_shape.width * gain) / 2, (new_shape.height - old_shape.height * gain) / 2);

            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };

            // scale coords on keypoint
            for (const auto& it : coords)
            {
                cv::Point2f temp{};
                temp.x = clamp((it.x - pad.first) / gain, 0, new_shape.width);
                temp.y = clamp((it.y - pad.second) / gain, 0, new_shape.height);
                scale_coords_keypt.push_back(temp);
            }
            return scale_coords_keypt;
        }
        
        /**
        * @fun outter round
        * @details round num into Three digits after Decimal separator
        */
        std::string outter_round(float& num)
        {
            auto str = std::to_string(num);
            str = str.substr(0, str.find("."));
            return str;
        }

        /**
        * @fun innner round 
        * @details round num into Three digits after Decimal separator
        */
        std::string inner_round(float& num)
        {
            auto str = std::to_string(std::round(num * 100) / 100);
            str = str.substr(0, str.find(".") + 3);
            return str;
        }

        /**
         * @fun calculation
         * @param boxes     包含表盘的表盘起点坐标（Xs, Ys)，终点坐标（Xe, Ye)，中心点坐标（Xc, Yc), 指针坐标（Xp, Yp)
         * @param limit     limit of gage
         * @param type      type of gage
         * @return result: 表盘指针与起点或者终点形成的角，占起点和终点形成的角的百分比
         */
        std::string calculation(std::vector<cv::Point2f>& boxes, std::pair<int,int>& cover)
        {
            float zero = 0;
            // cover judge
            //std::pair<int, int> cover_limit;
            //cover_limit = cover;

            //std::vector<cv::Point2f> contour = { boxes[0], boxes[1], boxes[2] };

            //auto area = cv::contourArea(contour);

            //if (area <= cover_limit.first || area >= cover_limit.second)
            //{
            //    return "999";
            //}
            std::cout << "LOG: 0 \n";
            std::cout << "boxes size:"<< boxes.size() <<"\n";

            float Xs = boxes[0].x;
            float Ys = boxes[0].y;

            float Xe = boxes[2].x;
            float Ye = boxes[2].y;

            float Xc = boxes[3].x;
            float Yc = boxes[3].y;

            float Xp = boxes[4].x;
            float Yp = boxes[4].y;

            std::cout << "LOG: 1 \n";
            // find distance between p and s
            float d1 = std::sqrt((Xp - Xs) * (Xp - Xs) + (Yp - Ys) * (Yp - Ys));

            // judge point at start point
            if (d1 < 10)
            {  
                return inner_round(zero);
            }

            // find distance between p and e
            float d2 = std::sqrt((Xp - Xe) * (Xp - Xe) + (Yp - Ye) * (Yp - Ye));

            std::cout << "LOG: 2 \n";

            if (d1 <= d2)
            {
                // pointer on the left
                float k1 = (Yc - Ys) / (Xc - Xs + 1);

                float b1 = Yc - k1 * Xc;

                float Y1p = k1 * Xp + b1;

                if (Y1p > Yp)
                {
                    // 指针与起点-表盘中心连线的夹角
                    float a = std::sqrt((Xp - Xs) * (Xp - Xs) + (Yp - Ys) * (Yp - Ys));
                    float b = std::sqrt((Xc - Xs) * (Xc - Xs) + (Yc - Ys) * (Yc - Ys));
                    float c = std::sqrt((Xc - Xp) * (Xc - Xp) + (Yc - Yp) * (Yc - Yp));

                    float cosA = (b * b + c * c - a * a) / (2 * b * c);

                    float A = acos(cosA);

                    // 起点-表盘中心连线与终点-表盘中心连线的夹角
                    float d = std::sqrt((Xs - Xe) * (Xs - Xe) + (Ys - Ye) * (Ys - Ye));
                    float e = std::sqrt((Xc - Xe) * (Xc - Xe) + (Yc - Ye) * (Yc - Ye));

                    float cosB = (b * b + e * e - d * d) / (2 * b * e);

                    float B = acos(cosB);

                    std::cout << "LOG: 3 \n";

                    float result;
                    float angle_ratio = A / (2 * PI - B);
                    result = angle_ratio * (100 - 0) + 0;
                    return inner_round(result);

                }
                else if (Y1p <= Yp)
                {
                    return inner_round(zero);
                }
            }
            else if (d1 > d2)
            {
                // 计算表盘终点-中心点连线，并将指针端点映射到连线上得到映射点，比较映射点和指针端点的高
                float k2 = (Yc - Ye) / (Xc - Xe + 1);

                float b2 = Yc - k2 * Xc;
                float Y2p = k2 * Xp + b2;
                if (Y2p > Yp)
                {
                    // 指针与起点-表盘中心连线的夹角
                    float a = std::sqrt((Xp - Xe) * (Xp - Xe) + (Yp - Ye) * (Yp - Ye));
                    float b = std::sqrt((Xc - Xe) * (Xc - Xe) + (Yc - Ye) * (Yc - Ye));
                    float c = std::sqrt((Xc - Xp) * (Xc - Xp) + (Yc - Yp) * (Yc - Yp));

                    float cosA = (b * b + c * c - a * a) / (2 * b * c);

                    float A = acos(cosA);

                    // 起点-表盘中心连线与终点-表盘中心连线的夹角
                    float d = std::sqrt((Xc - Xs) * (Xc - Xs) + (Yc - Ys) * (Yc - Ys));
                    float e = std::sqrt((Xs - Xe) * (Xs - Xe) + (Ys - Ye) * (Ys - Ye));

                    float cosB = (d * d + b * b - e * e) / (2 * d * b);

                    float B = acos(cosB);

                    std::cout << "LOG: 3 \n";

                    float result;
                    float angle_ratio = (2 * PI - A - B) / (2 * PI - B);
                    result = angle_ratio * (100 - 0) + 0;
                    return inner_round(result);

                }
                else if (Y2p <= Yp)
                {
                    return inner_round(zero);
                }
            }
        }

        /**
        * @fun scale_coords
        */
        cv::Point2f scale_coords(const cv::Point2f& pt, cv::Size& input_shape, cv::Size& output_shape)
        {
            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };
            // gain
            float gain = std::min(input_shape.width / (float)output_shape.width, input_shape.height / (float)output_shape.height);

            // pad
            float pad_w = (input_shape.width - output_shape.width * gain) / 2.0;
            float pad_h = (input_shape.height - output_shape.height * gain) / 2.0;

            float x1 = (pt.x - pad_w) / gain;
            float y1 = (pt.y - pad_h) / gain;

            clamp(x1, 0, output_shape.width);
            clamp(y1, 0, output_shape.height);

            cv::Point2f scale_pt = cv::Point2f(x1, y1);

            return scale_pt;
        }

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<needledash::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::tuple<std::string, cv::Point2f, cv::Point2f> run_detect(cv::Mat& image, int type, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                    {"cover_min", param_map.count("cover_min") ? param_map["cover_min"] : 1},
                    {"cover_max", param_map.count("cover_max") ? param_map["cover_max"] : 100000}};

            if (type != 3)
                return {"999", cv::Point2f(0,0),  cv::Point2f(0,0)};

            cv::Size input_Size = cv::Size(640, 640);
            cv::Size output_Size = cv::Size(image.cols, image.rows);

            // preprocess
            cv::Mat blob;
            float ratio;
            std::tie(blob, ratio) = preprocess(image);

            // cover min & cover max
            int cover_min = static_cast<int>(params["cover_min"] / (ratio * ratio));
            int cover_max = static_cast<int>(params["cover_max"] / (ratio * ratio));

            auto cover = std::make_pair(cover_min, cover_max);

            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_blob(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, blob.rows, blob.cols, 3}, -1, glasssix::memory::NHWC));
            // mat convert into tensor
            std::copy(blob.data, blob.data + blob.step[0] * blob.rows, input_tensor_blob->mutable_cpu_data());

            // NHWC into NCHW tensor
            input_tensor_blob->convert_order();
            auto input_tensor = input_tensor_blob | glasssix::memory::tensor_convert_to<float>;

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = meter_sim_instance_->forward(input_tensor);

            std::vector<std::string>  out_names = { "754","807","860","913" };

            for (size_t i = 0; i < out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }

            float conf_threshold = 0.5f;
            float iou_threshold = 0.5f;

            std::vector<pt> pt_location;
            std::vector<keypt> key_location;

            std::tie(pt_location, key_location) = concat(forwards, conf_threshold);

            std::cout << "Concat: detect size:" << pt_location.size() << "\n";

            // non_max_suppression
            std::vector<point> pt_nms;
            std::vector<keypt> key_nms;
            std::tie(pt_nms, key_nms) = non_max_suppression(pt_location, key_location, conf_threshold, iou_threshold);

            std::cout << "non_max_suppression pt_nms size:" << pt_nms.size() << "\n";

            // turn keypt into Point2f
            std::vector<cv::Point2f> key_point;
            for(auto &it : key_nms)
            {
                key_point.emplace_back(cv::Point2f(it.x, it.y));
            }

            std::string meter_result = calculation(key_point, cover);

            auto center_pt = scale_coords(key_point[3], input_Size, output_Size);
            auto point_pt = scale_coords(key_point[4], input_Size, output_Size);

            return { meter_result, center_pt, point_pt };
        }


    private:
        std::string model_directory_;
        int device_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> meter_sim_instance_;
    };

    ocr_code_internal::ocr_code_internal(std::string_view model_directory, int device)
            : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    ocr_code_internal::~ocr_code_internal()
    {
    }

    std::string ocr_code_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<needledash::box_info> ocr_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int type,
                                                                      int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, type, roi_x, roi_y, roi_width, roi_height, param_map);
    }

}

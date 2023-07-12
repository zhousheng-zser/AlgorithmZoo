#include <iostream>
#include <cmath>
#include <tuple>

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <RKNN2Wrapper/rknn2_wrapper.hpp>

#include <opencv2/dnn.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace glasssix::pedestrian_labor
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("pedestrian_labor_sim", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :detect_instance_(phai,  model_directory + std::string("/pedestrian_labor_sim.rknn"), device), classify_instance_(phai,  model_directory + std::string("/pedestrian_labor_classify_sim.rknn"), device)
        {

        }

        exposing::param_vector<pedestrian_labor::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
            
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in pedestrian_labor");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<pedestrian_labor::box_info>();

            for(auto& it:result) {
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = detect_instance_.version();
#else
			std::string nn_frame_version = detect_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:

        struct bbox
        {
            int x1;
            int y1;
            int x2;
            int y2;
            float score;
            int category;
        };

        struct pt {
            float x;
            float y;
            float w;
            float h;
            float score;
            int category;
        };

        // sigmoid
        static inline float sigmoid_x(float x) {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        // letterbox
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
         * @brief concat
         * @param outs      : excalibur inference output
         * @param conf_thres
         * @return  pt_location, key_location
         */
        std::vector<pt> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs, float conf_thres)
        {
            const float anchors[3][6] = {
                {10,13, 16,30, 33,23},
                {30,61, 62,45, 59,119},
                {116,90, 156,198, 373,326},
            };
            const float stride[3] = { 8.0, 16.0, 32.0 };

            std::vector<pt> pt_location;

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
                            const float* pdata = ptr_out + ind * 22;

                            float box_score = sigmoid_x(pdata[4]);

                            if (box_score >= conf_thres)
                            {
                                pt pt_temp{};
                                pt_temp.x = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                                pt_temp.y = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                                pt_temp.w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                                pt_temp.h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h
                                pt_temp.score = sigmoid_x(pdata[4]);
                                pt_temp.category = 0;

                            }

                            ind++;
                        }
                    }
                }
            }

            return pt_location;
        }

        /**
         * @fun computNmsInput
         * @param src, max_wh
         * @return bboxes, scores, category
         * @details slice src into bboxes and confidence, which need by dnn::NMS
         */
        static std::tuple<std::vector<cv::Rect2d>, std::vector<float>, std::vector<int>> computeNmsInput(std::vector<pt>& src)
        {
            std::vector<cv::Rect2d> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for(auto const &it: src)
            {
                cv::Rect2d temp;
                temp.x      = static_cast<double>(it.x);
                temp.y      = static_cast<double>(it.y);
                temp.width  = static_cast<double>(it.w);
                temp.height = static_cast<double>(it.h);
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
        std::vector<bbox> non_max_suppression(std::vector<pt>& pt_location, float conf_thres, float iou_thres)
        {
            std::vector<bbox> pt_nms;

            std::vector<cv::Rect2d> boxes;
            std::vector<float> scores;
            std::vector<int> category;

            std::tie(boxes, scores, category) = computeNmsInput(pt_location);

            std::vector<int> indices;
            cv::dnn::NMSBoxes(boxes, scores, conf_thres, iou_thres, indices, 1.f, 1);

            for(auto const &it: indices)
            {
                bbox pt_temp{};

                pt_temp.x1 = pt_location[it].x - pt_location[it].w / 2;
                pt_temp.y1 = pt_location[it].y - pt_location[it].h / 2;

                pt_temp.x2 = pt_location[it].x + pt_location[it].w / 2;
                pt_temp.y2 = pt_location[it].y + pt_location[it].h / 2;

                pt_temp.score = pt_location[it].score;

                pt_temp.category = pt_location[it].category;

                pt_nms.push_back(pt_temp);
            }

            return pt_nms;
        }

        /**
         * @fun scale_coords
         * @param coords, old_image, new_image, step
         * @return coords
         */
        std::vector<bbox> scale_coords(std::vector<bbox>& coords, cv::Size& old_shape, cv::Size& new_shape)
        {
            std::vector<bbox> scale_coords_pt;

            auto gain = std::min((float)old_shape.width / (float)new_shape.width, (float)old_shape.height / (float)new_shape.height);

            auto pad = std::make_pair((old_shape.width - new_shape.width * gain) / 2, (old_shape.height - new_shape.height * gain) / 2);

            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };

            // scale coords on point
            for (const auto& it : coords)
            {
                bbox temp{};
                temp.x1 = clamp((it.x1 - pad.first) / gain, 0, new_shape.width);
                temp.y1 = clamp((it.y1 - pad.second) / gain, 0, new_shape.height);
                temp.x2 = clamp((it.x2 - pad.first) / gain, 0, new_shape.width);
                temp.y2 = clamp((it.y2 - pad.second) / gain, 0, new_shape.height);
                temp.score = it.score;
                temp.category = 0;
                scale_coords_pt.push_back(temp);
            }

            return scale_coords_pt;
        }

        /**
         *  @fun find_max
         *  @param outs
         *  @return category
         */
        int classify(std::vector<std::shared_ptr<memory::tensor<float>>>& outs)
        {
            float output[26] = {0};

            auto max = [](float a, float b) { if(a > b) return a; else return b; };

            for(int i = 0; i < 4; i++)
            {
                const float* ptr_out = outs[i]->cpu_data();

                for(int i = 0; i < 26; i++)
                {
                    output[i] = max(output[i], ptr_out[i]);
                }
            }

            int category = 0;
            float threshold = 0.65f;
            if(output[9] > threshold)
                category += 1;
            if(output[11] > threshold)
                category += 10;
            if(output[12] > threshold)
                category += 100;

            return category;   
        }
    

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<pedestrian_labor::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<pedestrian_labor::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                    {"conf_thres", param_map.count("conf_thres") ? param_map["conf_thres"] : 0.65f},
                    {"iou_thres",  param_map.count("iou_thres") ? param_map["iou_thres"] : 0.45f}};
			
            cv::Mat blob;
            float ratio;
            std::tie(blob, ratio) = preprocess(image);

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = detect_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  detect_out_names={"output","288","302"};         // 8 - 16 -32


            for (size_t i=0;i< 3; i++)//对输出数据做处理
            {
                forwards.push_back(network_result[detect_out_names[i]]);
            }

			float conf_threshold = param_map["conf_thres"];
			float iou_threshold  = param_map["iou_thres"];

			auto result = concat(forwards, conf_threshold);

            auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold);

            auto old_shape = cv::Size(image.cols, image.rows);
            auto new_shape = cv::Size(blob.cols, blob.rows);

            auto scale_box = scale_coords(nms_result, new_shape, old_shape);

            // classify 
            std::vector<pedestrian_labor::box_info_internal> classify_res;
            
            for(const auto &it: scale_box)
            {
                pedestrian_labor::box_info_internal box_info;

                // cut roi
                cv::Mat roi_copy = image(cv::Rect(it.x1, it.y1, it.x2 - it.x1, it.y2 - it.y1));

                // classify preprocess
                cv::Mat temp_blob;

                // resize
                cv::resize(roi_copy, temp_blob, cv::Size(256, 512));
                
                std::vector<std::shared_ptr<memory::tensor<float>>> classify_forwards;

                auto  classify_network_result = classify_instance_.forward(temp_blob.data, { 1, temp_blob.rows, temp_blob.cols, temp_blob.channels() }, RKNN_TENSOR_NHWC);

                std::vector<std::string>  classify_out_names={"1648","2635","3107","predict"};   // 1-26

                for (size_t i=0;i< 3; i++)//对输出数据做处理
                {
                    classify_forwards.push_back(classify_network_result[classify_out_names[i]]);
                }

                box_info.x1 = it.x1;
                box_info.y1 = it.y1;
                box_info.x2 = it.x2;
                box_info.y2 = it.y2;
                box_info.score = it.score;
                box_info.category = classify(classify_forwards);
            
                classify_res.push_back(box_info);
            }

            return classify_res;
        }


    private:
        std::string model_directory_;
        int device_;
        rknnwrapper::rknn_wrapper detect_instance_;
        rknnwrapper::rknn_wrapper classify_instance_;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<pedestrian_labor::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

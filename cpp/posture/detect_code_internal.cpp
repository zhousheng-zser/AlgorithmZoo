#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "hardcode.hpp"
#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"
#include "Excalibur/operation_rotate.hpp"
#include "Primitives/tensor_conversions.hpp"

#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::posture
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {
           net_instance_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
           std::string(model_directory) + "/" +"Trespass_kpt_sim.racy", device);      
        }

        exposing::param_vector<posture::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in posture");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

             cv::Mat blob;
            float ratio;
            std::tie(blob, ratio) = preprocess(cropped_image);
    

            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_blob(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, blob.rows, blob.cols, 3}, -1, glasssix::memory::NHWC));
            // mat convert into tensor
            std::copy(blob.data, blob.data + blob.step[0] * blob.rows, input_tensor_blob->mutable_cpu_data());

            // NHWC into NCHW tensor
            input_tensor_blob->convert_order();
            auto input_tensor = input_tensor_blob | glasssix::memory::tensor_convert_to<float>;
			auto  network_result = net_instance_->forward(input_tensor);

            
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
            std::vector<std::string>  out_names = {"output", "701","723", "745"};

            for (size_t i = 0; i < out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }



			float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.1f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;      


            std::vector<pt>     pt_location;
            std::vector<keypt> key_location;

            std::tie(pt_location, key_location) = concat(forwards, conf_threshold);

            // non_max_suppression
            std::vector<point> pt_nms;
            std::vector<keypt> key_nms;
            std::tie(pt_nms, key_nms) = non_max_suppression(pt_location, key_location, conf_threshold, iou_threshold);



            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result;

            for (int i=0;i<pt_nms.size();i++)
            {

               box_info_internal temp_result;
                temp_result.x1=pt_nms[i].x1*ratio + roi_x;
                temp_result.y1=pt_nms[i].y1*ratio + roi_y;
                temp_result.x2=pt_nms[i].x2*ratio + roi_x;
                temp_result.y2=pt_nms[i].y2*ratio + roi_y;

                temp_result.score=pt_nms[i].score;

                temp_result.category=pt_nms[i].category;

                temp_result.key_points = exposing::make_param_vector<float>();

                for(int j=0;j<14;j++)
                {
                    temp_result.key_points.push_back(key_nms[j+14*i].x*ratio+roi_x);
                    temp_result.key_points.push_back(key_nms[j+14*i].y*ratio+roi_y);
                    temp_result.key_points.push_back(key_nms[j+14*i].score);
   
                }
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
            {
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));
            }

            return fin_result;


    
        }


        std::string version()
        {
            const std::string algo_module_version = "1.0.0";


            std::string nn_frame_version = net_instance_->version();

            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:


            struct point {
                float x1;
                float y1;
                float x2;
                float y2;
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

            struct keypt {
                float x;
                float y;
                float score;
            };



        std::pair<std::vector<pt>, std::vector<keypt>> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs, float conf_thres)
            {
                const float anchors[4][6] = { {19,27,  44,40,  38,94} , { 96,68,  86,152,  180,137},  {140,301,  303,264,  238,542},{ 436,615,  739,380,  925,792 }                                                              
                                                };
                const float stride[4] = { 8.0, 16.0, 32.0, 64.0 };

                std::vector<pt> pt_location;
                std::vector<keypt> key_location;
                auto class_pred = [](float x, float y) {if (x > y) return 1; else return 0; };

                for (int n = 0; n < 4; n++)
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
                                const float* pdata = ptr_out + ind * 48;

                                float box_score = sigmoid_x(pdata[4]);
                                float xxx = sigmoid_x(pdata[0]);

                                //if (box_score >= conf_thres)
                                if (box_score >= 0.1)
                                {
                                    pt pt_temp{};
                                    pt_temp.x = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx                 
                                    pt_temp.y = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                                    pt_temp.w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                                    pt_temp.h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h
                                    pt_temp.score = box_score;
                                        
                                    if (pt_temp.x > 270 && pt_temp.x < 290 && pt_temp.y>300)
                                    {
                                        int sd = 10;
                                    }

                                    pt_temp.category = class_pred(sigmoid_x(pdata[5]), sigmoid_x(pdata[6]));

                                    pt_location.push_back(pt_temp);

                                    //key-points
                                    for (int group = 0; group < 14; ++group) {
                                        keypt ky_temp{};
                                        ky_temp.x = (pdata[6 + group * 3] * 2.f - 0.5f + j) * stride[n]; //point x
                                        ky_temp.y = (pdata[7 + group * 3] * 2.f - 0.5f + i) * stride[n]; //point y
                                        ky_temp.score = sigmoid_x(pdata[8 + group * 3]); //point score

                                        key_location.push_back(ky_temp );
                                    }

                                }

                                ind++;
                            }
                        }
                    }
                }

                return std::make_pair(pt_location, key_location);
            }

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
                pt_temp.score = pt_location[it].score;
                pt_temp.category = pt_location[it].category;

                pt_nms.push_back(pt_temp);

                for (int i = 0; i < 14; ++i)
                {
                    key_nms.push_back(key_location[it * 14 + i]);
                }
            }

            return std::make_pair(pt_nms, key_nms);
        }

        static std::pair<cv::Mat, float> letterbox(cv::Mat& img) 
        {
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
                    cv::copyMakeBorder(resize_img, resize_img, 0, pad2+pad1, 0, 0, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });
        /*           cv::copyMakeBorder(resize_img, resize_img, pad1,  pad2, 0, 0, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });*/
                }
                else {
                    ratio = ratio_h;
                    int new_y = new_shape.height;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((new_shape.width - new_x) / 2);
                    int pad2 = new_shape.width - new_x - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0,  pad2+ pad1, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });

            /*       cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });*/
                }
            }

            return { resize_img, ratio };
        }

        std::pair<cv::Mat, float> preprocess(cv::Mat& image) 
        {
            // letterbox
            cv::Mat crop_image;
            float ratio;
            std::tie(crop_image, ratio) = letterbox(image);

            // cvt BGR2RGB
            cv::Mat rgb_image;
            cv::cvtColor(crop_image, rgb_image, cv::COLOR_BGR2RGB);

            return std::make_pair(rgb_image, ratio);
        }

        void xywh2xyxy(std::vector<std::array<float, 48>>& NMS_result, cv::Size origin_shape, std::tuple<float, float, int, int, int, int >& imgprocess_info) 
        {
            auto val_in_boundary = [](float val, float min_val, float max) {
                if (val < min_val + 1)
                    return min_val + 1;
                else if (val >= max - 1)
                    return min_val - 1;
                else
                    return val;
            };

            float ratio_w = std::get<0>(imgprocess_info);
            float ratio_h = std::get<1>(imgprocess_info);
            float col_pad_start = std::get<2>(imgprocess_info);
            float row_pad_start = std::get<4>(imgprocess_info);

            for (auto& box : NMS_result) {
                float cx = (box[0] - col_pad_start) * ratio_w;
                float cy = (box[1] - row_pad_start) * ratio_h;
                float w = box[2] * ratio_w;
                float h = box[3] * ratio_h;
                box[0] = val_in_boundary(cx - w / 2, 0, origin_shape.width);
                box[1] = val_in_boundary(cy - h / 2, 0, origin_shape.height);
                box[2] = val_in_boundary(cx + w / 2, 0, origin_shape.width);
                box[3] = val_in_boundary(cy + h / 2, 0, origin_shape.height);

                for (int kp_group = 0; kp_group < 14; ++kp_group) {
                    box[6 + kp_group * 3] = val_in_boundary((box[6 + kp_group * 3] - col_pad_start) * ratio_w, 0, origin_shape.width);
                    box[7 + kp_group * 3] = val_in_boundary((box[7 + kp_group * 3] - row_pad_start) * ratio_h, 0, origin_shape.height);
                }
            }
        }

        static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

       

    private:
        std::string model_directory_;
        int device_;
        std::unique_ptr < glasssix::excalibur::pipeline<float>> net_instance_;
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

    exposing::param_vector<posture::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
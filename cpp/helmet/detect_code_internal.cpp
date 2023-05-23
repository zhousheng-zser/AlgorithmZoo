#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

namespace glasssix::helmet
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("helmet", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/helmet_sim.rknn"), device)
        {

        }

        exposing::param_vector<helmet::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            // show bitmap channels and size
             std::cout<<"bitmap size: "<<bitmap.size()<<"\n";
             std::cout<<"bitmap channels: "<<channels<<"\n";
             std::cout<<"bitmap height: "<<height<<"\n";
             std::cout<<"bitmap width: "<<width<<"\n";

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            auto result = run_detect(image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<helmet::box_info>();

            for(const auto& it:result) {
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        std::tuple<cv::Mat, float> preprocess(cv::Mat img, int hope_size = 640)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_size;
            float ratio_h = (float)H / (float)hope_size;
            float ratio = ratio_w;
            //   std::cout<<"in refvest_imgprocess1\n";
            // for (size_t i = 0; i < 10000; i+=100)
            // {
            //    std::cout<<(int)img.data[i]<<"\t";
            // }
            // std::cout<<"\n\n";
            cv::Mat resize_img;
            if(H==hope_size && W==hope_size )
            {
                resize_img=img;
            }
            else
            {
                if (ratio_w == ratio_h)
                {
                    // std::cout<<"in refvest_imgprocess2\n";
                    // std::cout<<hope_size<<"\n";
                    cv::resize(img, resize_img, cv::Size2i{ hope_size, hope_size });}
                else if (ratio_w > ratio_h) {

                    int new_x = hope_size;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((hope_size - new_y) / 2);
                    int pad2 = hope_size - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });

                    cv::copyMakeBorder(resize_img, resize_img, 0, pad1 + pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
                else {

                    ratio = ratio_h;
                    int new_y = hope_size;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((hope_size - new_x) / 2);
                    int pad2 = hope_size - new_x - pad1;

                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    // std::cout<<"in refvest_imgprocess444\n";
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, pad1 + pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        /*
         * @fun yolo_decode
         * @param prediction, anchors, num_classes, input_shape, image_shape
         */
        std::vector<std::array<float, 7>> yolo_decoder(std::shared_ptr<memory::tensor<float>>& prediction, float conf_thres)
        {
            std::vector<std::array<float, 7>> detections_target;
            std::vector<std::array<float, 7>> detections_target_NMS;

            std::vector<cv::Rect2d> bboxes;
            std::vector<float> bbox_scores;

            float *data_ptr = prediction->cpu_data();

            for(int idx = 0; idx < 8400; idx++)
            {
                float obj_conf   = data_ptr[4];
                float false_conf = data_ptr[5];
                float true_conf  = data_ptr[6];
                bool class_pred  = true_conf > false_conf;
                float class_conf = data_ptr[5 + int(class_pred)];

                if (obj_conf * class_conf > conf_thres)
                {
                    if (idx > 8000) {
                        const int grid = 20;
                        const int stride = 32;

                        float center_x = (data_ptr[0] + (idx - 8000) % grid) * stride;
                        float center_y = (data_ptr[1] + (idx - 8000) / grid) * stride;
                        float box_w = std::exp(data_ptr[2]) * stride;
                        float box_h = std::exp(data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_conf);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_conf, class_conf, static_cast<float>(class_pred) });
                    }
                    else if (idx > 6400) {
                        const int grid = 40;
                        const int stride = 16;
                        float center_x = (data_ptr[0] + (idx - 6400) % grid) * stride;
                        float center_y = (data_ptr[1] + (idx - 6400) / grid) * stride;
                        float box_w = std::exp(data_ptr[2]) * stride;
                        float box_h = std::exp(data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_conf);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_conf, class_conf, static_cast<float>(class_pred) });
                    }
                    else {
                        const int grid = 80;
                        const int stride = 8;
                        float center_x = (data_ptr[0] + (idx - 0) % grid) * stride;
                        float center_y = (data_ptr[1] + (idx - 0) / grid) * stride;
                        float box_w = std::exp(data_ptr[2]) * stride;
                        float box_h = std::exp(data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_conf);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_conf, class_conf, static_cast<float>(class_pred) });
                    }
                }
                data_ptr+=7;
            }

            // NMS
            std::vector<int> bbox_indices;
            cv::dnn::NMSBoxes(bboxes, bbox_scores, 0.5, 0.5, bbox_indices);

            for (int i = 0; i < bbox_indices.size(); i++) {
                detections_target_NMS.push_back(detections_target[bbox_indices[i]]);
            }

            return detections_target_NMS;
        }

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<helmet::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<helmet::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            auto old_shape = cv::Size(image.cols, image.rows);
            auto new_shape = cv::Size(640, 640);

            // show detect input image size
            std::cout << "detect input image size: " << image.cols << "x" << image.rows << std::endl;

            cv::Mat blob;
            float ratio = 0;
            std::tie(blob, ratio) = preprocess(image);

            // show preprocess input image size
            std::cout << "preprocess input image size: " << blob.cols << "x" << blob.rows << std::endl;

            auto  output = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            float conf_thres = 0.3f;

            std::vector<std::array<float, 7>> detections = yolo_decoder(output["output"], conf_thres);

            // bbox into box_info_internal
            std::vector<box_info_internal> result;

            for (const auto& bbox : detections) {
                box_info_internal box_info;
                box_info.x1 = static_cast<int>(bbox[0] * ratio);
                box_info.y1 = static_cast<int>(bbox[1] * ratio);
                box_info.x2 = static_cast<int>(bbox[2] * ratio);
                box_info.y2 = static_cast<int>(bbox[3] * ratio);
                box_info.category = static_cast<int>(bbox[6]);
                result.push_back(box_info);
            }

            return result;
        }


    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper net_instance_;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<helmet::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
#include <iostream>
#include <cmath>
#include <tuple>


#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::refvest
{
    class classify_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("refvest", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/refvest_sim.rknn"), device)
        {

        }

        exposing::param_vector<refvest::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            std::vector<box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in refvest");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));
            
            run_refvest(results, cropped_image);
            for (auto& i : results)
            {
                i.x1+=roi_x;
                i.x2+=roi_x;
                i.y1+=roi_y;
                i.y2+=roi_y;
                
                i.x1= i.x1>0?i.x1:0;
                i.x1= i.x1<width?i.x1:width;

                i.x2= i.x2>0?i.x2:0;
                i.x2= i.x2<width?i.x2:width;

                i.y1= i.y1>0?i.y1:0;
                i.y1= i.y1<height?i.y1:height;

                i.y2= i.y2>0?i.y2:0;
                i.y2= i.y2<height?i.y2:height;

          

                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }          
            return result;
            
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_instance_.version();
#else
			std::string nn_frame_version = net_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:

        void run_refvest(std::vector<box_info_internal>& result, cv::Mat& image)
        {
            auto [det_mat ,ratio]= refvest_imgprocess(image, 640);
            if(det_mat.empty())
            {
                std::cout<<"det_mat empty\n";
            }


            //cv::Mat blob = cv::dnn::blobFromImage(det_mat);
            cv::Mat blob= det_mat;

            std::chrono::time_point<std::chrono::system_clock> timer_start;

            std::vector<cv::Mat> forwards;
            if(blob.empty())
            {
                std::cout<<"blob empty\n";
            }
          
            auto  output = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::array<float, 7>> detections = refvest_yolo_decoder(output["output"]);
            std::vector<std::array<float, 7>> out = ppeople_refvest_assignment(detections);

            for (const auto& bbox : out) {
                box_info_internal box_ifo;
                box_ifo.x1 = static_cast<int>(bbox[0] * ratio);
                box_ifo.y1 = static_cast<int>(bbox[1] * ratio);
                box_ifo.x2 = static_cast<int>(bbox[2] * ratio);
                box_ifo.y2 = static_cast<int>(bbox[3] * ratio);
                box_ifo.score = bbox[4]* bbox[5];
                box_ifo.category = static_cast<int>(bbox[6]);

                result.push_back(box_ifo);
            }

        }

        std::vector<std::array<float, 7>> ppeople_refvest_assignment(std::vector<std::array<float, 7>>& detections)
        {
            std::vector<std::array<float, 7>> detections_people;
            std::vector<std::array<float, 7>> detections_ref;

            for (auto& detection : detections) 
            {
                if (detection[6] > 0.5) 
                {
                    detections_ref.push_back(detection);
                }
                else 
                {
                    detections_people.push_back(detection);
                }
            }

            for (auto& people : detections_people) {
                float people_c = people[2] + people[0];
                float people_w = (people[2] - people[0]) * 1.8;

                for (auto& ref : detections_ref) {
                    auto ref_c = ref[2] + ref[0];
                    if (std::abs(people_c - ref_c) < people_w) {
                        people[6] = 1;
                    }
                }
            }

            return detections_people;
        }

      std::vector<std::array<float, 7>> refvest_yolo_decoder(std::shared_ptr<memory::tensor<float>>& detectionMat, int type = 0)
        {
            std::vector<std::array<float, 7>> detections_target;
            std::vector<std::array<float, 7>> detections_target_NMS;

            std::vector<cv::Rect2d> bboxes;
            std::vector <float> bbox_scores;
            const float *data_ptr=detectionMat->cpu_data();
            for (int idx = 0; idx < 8400; idx++) {
                 
                float obj_confidence = data_ptr[4];
                float false_conf = data_ptr[5];
                float true_conf = data_ptr[6];
                bool class_pred = true_conf > false_conf;
                float class_conf = data_ptr[5 + int(class_pred)];


                if (obj_confidence * class_conf > 0.5f)
                {
                    if (idx > 8000) {
                        const int grid = 20;
                        const int stride = 32;

                        float center_x = (data_ptr[0] + (idx - 8000) % grid) * stride;
                        float center_y = (data_ptr[1] + (idx - 8000) / grid) * stride;
                        float box_w = std::exp(data_ptr[2]  ) * stride;
                        float box_h = std::exp(data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_confidence);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_confidence, class_conf, static_cast<float>(class_pred) });
                    }
                    else if (idx > 6400) {
                        const int grid = 40;
                        const int stride = 16;
                        float center_x = ( data_ptr[0] + (idx - 6400) % grid) * stride;
                        float center_y = (data_ptr[1] + (idx - 6400) / grid) * stride;
                        float box_w = std::exp(data_ptr[2]) * stride;
                        float box_h = std::exp(data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_confidence);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_confidence, class_conf, static_cast<float>(class_pred) });
                    }
                    else {
                        const int grid = 80;
                        const int stride = 8;
                        float center_x = ( data_ptr[0]+ (idx - 0) % grid) * stride;
                        float center_y = ( data_ptr[1]+ (idx - 0) / grid) * stride;
                        float box_w = std::exp( data_ptr[2]  ) * stride;
                        float box_h = std::exp( data_ptr[3]) * stride;

                        float v0 = center_x - box_w / 2;
                        float v1 = center_y - box_h / 2;
                        float v2 = center_x + box_w / 2;
                        float v3 = center_y + box_h / 2;

                        bboxes.push_back({ v0, v1 , box_w, box_h });
                        bbox_scores.push_back(obj_confidence);

                        detections_target.push_back({ v0, v1 , v2, v3, obj_confidence, class_conf, static_cast<float>(class_pred) });
                    }
                }
                           data_ptr+=7;
            }

            std::vector<int> bbox_indices;
            cv::dnn::NMSBoxes(bboxes, bbox_scores, 0.5, 0.5, bbox_indices);

            for (int i = 0; i < bbox_indices.size(); i++) {
                detections_target_NMS.push_back(detections_target[bbox_indices[i]]);
            }

            return detections_target_NMS;
        }
        
        std::pair<cv::Mat, float> refvest_imgprocess(const cv::Mat& img, int hope_size = 640) {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_size;
            float ratio_h = (float)H / (float)hope_size;
            float ratio = ratio_w;

            cv::Mat resize_img;
            if(H==hope_size && W==hope_size )
            {
                resize_img=img;
            }
            else
            {
                if (ratio_w == ratio_h)
                    {
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
                      
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, pad1 + pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper net_instance_;
    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    classify_code_internal::~classify_code_internal() = default;

    std::string classify_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<refvest::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height);
    }
}

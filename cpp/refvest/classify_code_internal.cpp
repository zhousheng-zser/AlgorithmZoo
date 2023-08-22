#include <iostream>
#include <cmath>
#include <tuple>

#include "../posture/detect_code.hpp"

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
                : impl{get_model_params("climb", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :classify_instance_(phai,  model_directory + std::string("/refvest_cls.rknn"), device), model_directory_(model_directory)
        {
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device);
        }       

        exposing::param_vector<refvest::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            std::vector<refvest::box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in refvest");
            }

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, empty_map_abi);
            
            std::vector<PostureInfo> persons_info;

            for (auto pinfo : posture_info_list) {

                PostureInfo postureInfo{ pinfo };

                // get key 5 - key 13 min xy max xy
                std::vector<float> x_vec;
                std::vector<float> y_vec;
                for(int i = 5; i < 13; i++)
                {
                    x_vec.push_back(postureInfo.Kpoints[i].first.x);
                    y_vec.push_back(postureInfo.Kpoints[i].first.y);
                }

                auto limit = [](int x) {if (x < 0) return 0; else return x; };

                float min_x = *std::min_element(x_vec.begin(), x_vec.end());
                float min_y = *std::min_element(y_vec.begin(), y_vec.end());
                float max_x = *std::max_element(x_vec.begin(), x_vec.end());
                float max_y = *std::max_element(y_vec.begin(), y_vec.end());
                
                cv::Point min_pt(limit(min_x), limit(min_y));
                cv::Point max_pt(limit(max_x), limit(max_y));

                cv::Mat cropped_image = image(cv::Range(min_pt.x, max_pt.x), cv::Range(min_pt.y, max_pt.y)).clone();
                
                auto classify_result = run_classify(cropped_image, param_map);

                auto classify_score = [](int x, int y) { if (x > y) return x; else return y;}; 
                auto classify_label = [](int x, int y) { if (x > y) return 0; else return 1;};
                
                refvest::box_info_internal box_info;

                box_info.x1 = min_pt.x;
                box_info.y1 = min_pt.y;  
                box_info.x2 = max_pt.x;
                box_info.y2 = max_pt.y;
                box_info.score = classify_score(classify_result.first, classify_result.second);
                box_info.category = classify_label(classify_result.first, classify_result.second);

                results.push_back(box_info);
            }
			
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
			std::string nn_frame_version = classify_instance_.version();
#else
			std::string nn_frame_version = classify_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:
        /**
        * @fun letterbox
        * @param src, new_shape
        * @return tensor(preprocess(image))
        * @details image preprocess and make tensor from images
        */
        std::tuple<cv::Mat, float> letterbox(cv::Mat img, int hope_size = 640)
        {
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

                    cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
                else {

                    ratio = ratio_h;
                    int new_y = hope_size;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((hope_size - new_x) / 2);
                    int pad2 = hope_size - new_x - pad1;

                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });

                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        std::pair<float,float> run_classify(cv::Mat& image, std::map<std::string, float>& param_map)
        {   
            cv::Mat blobs;
            float ratio = 0;
            std::tie (blobs, ratio) = letterbox(image, 224);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

            auto  network_result = classify_instance_.forward(blobs.data, { 1, blobs.rows, blobs.cols, blobs.channels() }, RKNN_TENSOR_NHWC);

            forwards.push_back(network_result["output0"]);

            const float* data_ptr = forwards[0]->cpu_data();

            return std::make_pair(data_ptr[0], data_ptr[1]);

        }



    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper classify_instance_;
        posture::detect_code posture_instance_;
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

    exposing::param_vector<refvest::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

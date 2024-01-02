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
                : impl{get_model_params("refvest", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :classify_instance_(phai,  model_directory + std::string("/refvest_cls.rknn"), device), model_directory_(model_directory)
        {
            // static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            // posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device,1);
   
        }       

        exposing::param_vector<refvest::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map)
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


            float posture_conf_thres = param_map.count("conf_thres")?param_map["conf_thres"]:0.65;
            float posture_little_target_conf_thres = param_map.count("little_target_conf_thres")?param_map["little_target_conf_thres"]:0.15;
            float posture_nms_thres = param_map.count("iou_thres")?param_map["iou_thres"]:0.8;

            posture_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();//conf_thres
			posture_param_abi.add_or_update("conf_thres", posture_conf_thres);
			posture_param_abi.add_or_update("nms_thres", posture_nms_thres);
            posture_param_abi.add_or_update("little_target_conf_thres", posture_little_target_conf_thres);

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            // exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, posture_param_abi);
                    // std::cout<<"persons_info size():  "<<posture_info_list.size()<<std::endl;
            std::vector<PostureInfo> persons_info;

         
            for (auto pinfo : posture_info_list) 
            {    
                PostureInfo postureInfo{ pinfo };

                // get key 5 - key 13 min xy max xy
                std::vector<float> x_vec;
                std::vector<float> y_vec;

                std::vector<float> pred_vec = {postureInfo.Kpoints[5].second , postureInfo.Kpoints[6].second , postureInfo.Kpoints[11].second , postureInfo.Kpoints[12].second};

                int sum = 0;

                for(auto &it: pred_vec)
                {
                    if(it < 0.8) 
                    { 
                        sum += 1;
                    }
                }
                
                if(sum < 2)
                {
                    // std::cout << "post: ["<<postureInfo.x1 << ", " << postureInfo.y1 << "], [" << postureInfo.x2 << ", " << postureInfo.y2 << "], "<< postureInfo.score << "\n";
                    // valid
                    for(int i = 5; i < 13; i++)
                    {   
                        x_vec.push_back(postureInfo.Kpoints[i].first.x);
                        y_vec.push_back(postureInfo.Kpoints[i].first.y);
                    }

                    float min_x = *std::min_element(x_vec.begin(), x_vec.end());
                    float min_y = *std::min_element(y_vec.begin(), y_vec.end());
                    float max_x = *std::max_element(x_vec.begin(), x_vec.end());
                    float max_y = *std::max_element(y_vec.begin(), y_vec.end());

                    constexpr auto limit = [](float x) {if (x < 0) return 0.0f ; else return x; };

                    min_x = limit(min_x);
                    min_y = limit(min_y);                 
                    max_x = limit(max_x);
                    max_y = limit(max_y);

                    if(min_x > (image.cols - 1))
                    {
                        min_x = image.cols - 1;
                    }	
                    if(min_y > (image.rows - 1))
                    {
                        min_y = image.rows - 1;
                    }
                    if(max_x > (image.cols - 1))
                    {
                        max_x = image.cols - 1;	
                    }
                    if(max_y > (image.rows - 1))
                    {
                        max_y = image.rows - 1;
                    }

                    if((max_x - min_x < 0) || (max_y - min_y < 0) || (max_x - min_x == 0) ||(max_y - min_y == 0))
                    {
                        continue;
                    }
                    else
                    {
                        cv::Point min_pt(min_x, min_y);
                        cv::Point max_pt(max_x, max_y);
                        
                        cv::Mat cropped_image = image(cv::Range(min_pt.y, max_pt.y), cv::Range(min_pt.x, max_pt.x)).clone();
                        
                        auto classify_result = run_classify(cropped_image, param_map);

                        float score = 0.f;
                        int category = 0;

                        if(classify_result.first > classify_result.second)
                        {
                            score = classify_result.first;
                            category = 0;
                        }
                        else 
                        {
                            score = classify_result.second;
                            category = 1;
                        }

                        refvest::box_info_internal box_info;

                        box_info.x1 = min_pt.x;
                        box_info.y1 = min_pt.y;  
                        box_info.x2 = max_pt.x;
                        box_info.y2 = max_pt.y;
                        box_info.score = score;
                        box_info.category = category;

                        results.push_back(box_info);
                    }
                }
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
			const std::string algo_module_version = "2.2.0";

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
         std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, int& pad_h640, int& pad_w640,  cv::Size input_shape = cv::Size(224, 224) )
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);

            cv::Mat cut_image;

            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));

            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h640 = int((input_shape.height - cut_image.rows) /2 ) ; 

                pad_w640 = int((input_shape.width - cut_image.cols) /2 ) ; 

                cv::copyMakeBorder(cut_image, mask_image, pad_h640, input_shape.height-cut_image.rows-pad_h640, pad_w640, input_shape.width-cut_image.cols-pad_w640, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }

            else 
            {
                src.copyTo(mask_image);     
            }
            
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);

            return {mask_image,scale};

        }

        std::pair<float,float> run_classify(cv::Mat& image, std::map<std::string, float>& param_map)
        {   
            cv::Mat blobs;
            float ratio = 0;
            int pad_w=0;
            int pad_h=0;

            std::tie (blobs, ratio) = preprocess_detection(image, pad_h,pad_w);
            // cv::resize(image, blobs, cv::Size(224, 224));

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

            auto  network_result = classify_instance_.forward(blobs.data, { 1, blobs.rows, blobs.cols, blobs.channels() }, RKNN_TENSOR_NHWC);

            forwards.push_back(network_result["output0"]);

            const float* data_ptr = forwards[0]->cpu_data();

            return std::make_pair(data_ptr[0], data_ptr[1]);

        }



    private:
    	exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper classify_instance_;
        // posture::detect_code posture_instance_;
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

    exposing::param_vector<refvest::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
    }
}

#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <chrono>
#include "general.hpp"

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>
#include <GenPipeline/GenPipeline.hpp>
#include <Excalibur/pipeline.hpp>

namespace glasssix::batterypilferers
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

            std::vector<std::string> phai;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_battery_person_car_detect_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/batterypilferers_detect.rknn", device);
            net_pilferage_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/batterypilferers_class.rknn", device);   

#elif defined(USE_BMNN)
            net_battery_person_car_detect_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/batterypilferers_detect.bmodel", device);
            net_pilferage_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/batterypilferers_class.bmodel", device);   
#endif      
            net_battery_person_car_detect_->manual_possible_normalization(std::array<float,3>{0.f,0.f,0.f},std::array<float,3>{1.f / 255.f,1.f / 255.f,1.f / 255.f});
            yolov8_instance = std::make_shared<Yolov8<GenPipeline,true>>(1280,1280, net_battery_person_car_detect_);  
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.1";
			std::string nn_frame_version = "sddsd";
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }


        exposing::param_vector<batterypilferers::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
                                                        int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.3f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            // CHECK_EQ(channels, 24);

            std::vector<cv::Mat> images;
            for (size_t i = 0; i < batch_size; i++)
            {
                cv::Mat image(cv::Size(width, height), CV_8UC3, bitmap.data() + i * 3 * height * width);
                images.push_back(image);
            }
            
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in batterypilferers");
            }

            std::vector<std::vector<car_person_batery>> frames_info;
            for (size_t i = 0; i < batch_size; i++)
            {
                cv::Mat cropped_image = images[i](cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));
                std::vector<Bbox> one_frame_result;
                auto objects =  yolov8_instance->get_objects(cropped_image,con_thres,iou_thres);
                for (auto& object: objects)             
                    one_frame_result.emplace_back(object.x1, object.y1, object.x2, object.y2, object.category, object.score,0 );
                auto result = deal_one_frame(one_frame_result);
                for(auto x : one_frame_result)
                {
                    // cv::rectangle(cropped_image, cv::Point(x.x1, x.y1), cv::Point(x.x2, x.y2), cv::Scalar(x.category==2?255:0, x.category?255:0, x.category?0:255), 2);
                }
                cv::imwrite( std::to_string(i)+"detect.jpg" ,cropped_image);
                frames_info.push_back(result);
            }

            auto compareVectors = [](const std::vector<car_person_batery>& a, const std::vector<car_person_batery>& b) {
                return a.size() > b.size(); };

            std::sort(frames_info.begin(), frames_info.end(), compareVectors);

            std::vector<Bbox> crop_rect = get_candicate_rect(frames_info[0],frames_info[1]);

            std::vector<int> is_battery_pilferers(crop_rect.size());
            std::vector<float> scores(crop_rect.size());

            for (int i=0;i<crop_rect.size();i++) 
            { 
                std::cout<<  crop_rect[i].y1<<" "<<crop_rect[i].y2<<" "<<crop_rect[i].x1<<" "<<crop_rect[i].x2<<std::endl;
                std::vector<cv::Mat> candicate_images;
                for (size_t j = 0; j < batch_size; j++)
                {
                    cv::Mat candicate_detect = images[i*batch_size+j](cv::Range(crop_rect[i].y1, crop_rect[i].y2), cv::Range(crop_rect[i].x1, crop_rect[i].x2));
                    cv::resize(candicate_detect, candicate_detect, cv::Size(256, 256));
                    candicate_images.push_back(candicate_detect);    
                }

                std::vector<float> candicate_steal(65536*24);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                concat_pic<order::NHWC>(candicate_images,candicate_steal.data());
                auto  network_result = net_pilferage_->forward(candicate_steal.data(), { 1, 256, 256, 3*batch_size}, 1 ).begin()->second->mutable_cpu_data();
                std::cout<<network_result[0]<<"network_result: \n";
#else           
                concat_pic<order::NCHW>(candicate_images,candicate_steal.data());
                auto  network_result = net_pilferage_->forward(candicate_steal.data(), { 1, 3*batch_size, 256, 256}, 0 ).begin()->second->mutable_cpu_data();
#endif
                is_battery_pilferers[i] = network_result[0]>network_result[1] ? 1 : 0 ;
                scores[i]=network_result[0];
            }

            auto fin_result= exposing::make_param_vector<box_info>();
            std::vector<box_info_internal> result;
            for (int i=0; i < crop_rect.size(); i++)
            {
                box_info_internal temp_result;
                temp_result.x1=crop_rect[i].x1;
                temp_result.y1=crop_rect[i].y1;
                temp_result.x2=crop_rect[i].x1 + crop_rect[i].x2;
                temp_result.y2=crop_rect[i].y1 + crop_rect[i].y2;
                temp_result.score=  scores[i];
                temp_result.category = is_battery_pilferers[i];
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));

            return fin_result;
        }

    private:

        int batch_size=8; 

        std::string model_directory_;
        int device_; 

        std::shared_ptr<GenPipeline> net_battery_person_car_detect_;
        std::shared_ptr<GenPipeline> net_pilferage_;
        std::shared_ptr<Yolov8<GenPipeline, true>> yolov8_instance;

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

    exposing::param_vector<batterypilferers::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

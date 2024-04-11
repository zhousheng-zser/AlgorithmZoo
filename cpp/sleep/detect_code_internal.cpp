#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

namespace glasssix::sleep
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{ exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(std::string model_directory, int device)       
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_sleep_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/sleeping.rknn", device);
#elif defined(USE_BMNN)
            net_sleep_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/sleeping.bmodel", device);
#endif
            net_sleep_->manual_possible_normalization(std::array<float,3>{0.f,0.f,0.f},std::array<float,3>{1.f / 255.f,1.f / 255.f,1.f / 255.f});
            yolov8_instance = std::make_shared<Yolov8<GenPipeline,true, true>>(640,640, net_sleep_);

        }

        exposing::param_vector<sleep::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
                 
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                  throw exposing::abi_invalid_argument("incorrect roi in sleep");
            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);
            auto results = exposing::make_param_vector<sleep::box_info>();
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
			const std::string algo_module_version = "2.0.0";
			std::string nn_frame_version = net_sleep_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}


        std::vector<sleep::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
		
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.85f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      
            int  device_id = std::round(param_map.count("device_id") ? param_map["device_id"] : 0.f);      
            int  frame_count_thres = std::round(param_map.count("frame_count_thres") ? param_map["frame_count_thres"] : 10.f);      
            auto objects = yolov8_instance->get_objects( image, conf_threshold, iou_threshold );

            std::vector<box_info_internal> output;
            for(auto& var : objects)
            {
                box_info_internal temp;
                temp.x1 = var.x1;
                temp.y1 = var.y1;
                temp.x2 = var.x2;
                temp.y2 = var.y2;
                temp.category = var.category ;   //1是睡岗 0是其他
                temp.confidence = var.score  ;
                output.push_back(temp);
            }
            return output;
        }


    private:
        std::shared_ptr<GenPipeline> net_sleep_;
        std::shared_ptr<Yolov8<GenPipeline, true, true>> yolov8_instance;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<sleep::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}
    
    // std::map<int, std::vector<Sleep_trace>>  detect_code_internal::impl::sleep_trace_;

}

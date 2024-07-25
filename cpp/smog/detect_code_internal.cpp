#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <GenPipeline/GenPipeline.hpp>
    #include <YoloFamily/Yolo_wrapper.hpp>
#elif defined(USE_BMNN)
    #include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

namespace glasssix::smog
{
    class detect_code_internal::impl
    {
    public:
        impl() {}
        impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_pedestrian_ = std::make_shared<GenPipeline>(model_dir + "/smog.rknn", device);
            Yolov8_Complement_instance = std::make_shared<Yolov8_Complement<GenPipeline>>(640, 640, net_pedestrian_);
#elif defined(USE_BMNN)
            Yolov8_Complement_instance = std::make_shared<SophonYolov8Wrapper>( model_dir + "/smog.bmodel");
            Yolov8_Complement_instance->init();
#endif     
        }

        exposing::param_vector<smog::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
        
            if (bitmap.empty()) 
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(bitmap.size(), channels * height * width);
            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in universal_pedestrian");
            
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            auto smog_list = Yolov8_Complement_instance->get_objects(cropped_image, conf_thres);

            auto results = exposing::make_param_vector<smog::box_info>();
            for (auto& smog : smog_list)
            {
                box_info_internal smog_internal;
                smog_internal.x1 = smog.x1;
                smog_internal.y1 = smog.y1;
                smog_internal.x2 = smog.x2;
                smog_internal.y2 = smog.y2;
                smog_internal.confidence = smog.score;
                smog_internal.category = 1;
                results.push_back(exposing::make_as_first<box_info_impl>(smog_internal));
            }
            return results;
        }
     
		std::string version()
		{
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = "3.0.0";
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<GenPipeline> net_pedestrian_;
        std::shared_ptr<Yolov8_Complement<GenPipeline>> Yolov8_Complement_instance;
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> Yolov8_Complement_instance;
#endif

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<smog::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}

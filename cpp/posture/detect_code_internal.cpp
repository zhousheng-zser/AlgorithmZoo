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

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

namespace glasssix::posture
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device, int model_type)
            : model_directory_{ std::string(model_directory) }, device_{ device },model_type_{model_type}
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
           
            if(model_type_)
                    net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture960_17.rknn", device);
            else
                    net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture960_12.rknn", device);
           
#elif defined(USE_BMNN)
            if(model_type_==1)
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture960_17.bmodel", device);
            else
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture960_12.bmodel", device);
          
#endif
            yolov8_instance = std::make_shared<Yolov8<GenPipeline,false,true>>(960,960, net_posture_);
        }

std::string version()
        {
			const std::string algo_module_version = "3.1.0";
			std::string nn_frame_version = net_posture_->version();
			return fmt::format(R"({{"nn_frame_version":"{}  ", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }



        exposing::param_vector<posture::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.45f;
            float little_target_conf_thres = param_map.count("little_target_conf_thres") ? param_map["little_target_conf_thres"] : 0.2f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in posture");

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            auto objects = yolov8_instance->get_objects( image, con_thres, iou_thres );

            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result; //objects
            for(auto& object : objects )
            {
                box_info_internal temp_result;
                temp_result.x1 = object.x1 + roi_x; //body[0]+ roi_x;
                temp_result.y1 = object.y1 + roi_y;
                temp_result.x2 = object.x1 + roi_x; //body[0]+ roi_x;
                temp_result.y2 = object.y1 + roi_y;
                temp_result.score = object.score ;
                temp_result.key_points = exposing::make_param_vector<float>();
                for(int j=0;j<object.key_points.size();j++)
                {
                    temp_result.key_points.push_back(object.key_points[j].x + roi_x);
                    temp_result.key_points.push_back(object.key_points[j].y + roi_y);
                    temp_result.key_points.push_back(object.key_points[j].score);

                }
                result.push_back( temp_result  );
            }

            for (auto& i : result)
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));

            return fin_result;
        }


    private:
        std::string model_directory_;
        int device_; 
        int model_type_=1;
        int keypoint_num=17;

        std::shared_ptr<GenPipeline> net_posture_;
        std::shared_ptr<Yolov8<GenPipeline, false,true>> yolov8_instance;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device,int model_type)
        : impl_{ std::make_unique<impl>(model_directory, device, model_type) }
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

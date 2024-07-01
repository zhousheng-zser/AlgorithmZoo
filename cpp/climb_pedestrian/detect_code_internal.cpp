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

namespace glasssix::climb_pedestrian
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{exposing::to_narrow_string(model_directory), device}
        {
        }

        impl( std::string model_directory, int device)
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_climb_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_pedestrian.rknn", device);
#elif defined(USE_BMNN)
            net_climb_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_pedestrian.bmodel", device);
#else
            net_climb_ = std::make_shared<GenPipeline>(get_model_params("climb_20240426cut"), std::string(model_directory) + "/climb_20240426cut.racy", device);
#endif
            net_climb_->manual_possible_normalization(std::array<float,3>{104.f, 117.f, 123.f},std::array<float,3>{1.f/128.f, 1.f/128.f, 1.f/128.f});
        }

        exposing::param_vector<climb_pedestrian::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,  std::map<std::string, float>& param_map,const std::vector<PedestrianInfo> &pedestrain_info)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in climb_pedestrian");

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

   //         auto climb_objects = yolov8_instance->get_objects( image, con_thres, nms_thres );
   //         auto climb_objects = net_climb_->forward(image).begin()->second;
   //         auto tensor_out_data = tensor_out->mutable_cpu_data();
			//int index = std::max_element(result, result + 1) - result;
            auto results = exposing::make_param_vector<climb_pedestrian::box_info>();
            std::vector<climb_pedestrian::box_info_internal> boxs;
            for (auto pinfo : pedestrain_info) 
            {
                climb_pedestrian::box_info_internal box;
                box.x1 = pinfo.x1;
                box.y1 = pinfo.y1;
                box.x2 = pinfo.x2;
                box.y2 = pinfo.y2;
                //int x1,x2,y1,y2;
                //x1 = pinfo.x1;
                //x2 = pinfo.x2;
                //y1 = pinfo.y1;
                //y2 = pinfo.y2;
                cv::Mat body;
                cv::Mat crop = image(cv::Range(box.x1, box.y1), cv::Range(box.x2, box.y2)).clone();
                cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                cv::resize(crop, body, cv::Size((int)(80), (int)80), cv::INTER_CUBIC);
                auto climb_objects = net_climb_->forward(image).begin()->second->mutable_cpu_data();
                int index = std::max_element(climb_objects, climb_objects + 1) - climb_objects;
                box.confidence= climb_objects[index];
                box.category = index ;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(box));
            }
            return results;
        }

        std::string version()
		{
			const std::string algo_module_version = "2.0.0";
            std::string nn_frame_version = net_climb_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:
        std::string model_directory_;
        int device_;
        std::shared_ptr<GenPipeline> net_climb_;
        std::shared_ptr<Yolov8<GenPipeline, true, true>> yolov8_instance;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<climb_pedestrian::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map, const std::vector<PedestrianInfo>& pedestrain_info) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}

#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

#include "../posture/detect_code.hpp"
#include "climb_assisant.hpp"
namespace glasssix::climb
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("climb", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
        {
            // static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            // posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device,1);
        }

        exposing::param_vector<climb::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
                 
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in climb");
            }

            float x1= param_map.count("x1") ? param_map["x1"] : 0.3f;
            float y1 = param_map.count("y1") ? param_map["y1"] : 0.5f; 

            float x2= param_map.count("x2") ? param_map["x2"] : 0.3f;
            float y2 = param_map.count("y2") ? param_map["y2"] : 0.5f; 
            
            float x3= param_map.count("x3") ? param_map["x3"] : 0.3f;
            float y3 = param_map.count("y3") ? param_map["y3"] : 0.5f;   
            
            float x4= param_map.count("x4") ? param_map["x4"] : 0.3f;
            float y4 = param_map.count("y4") ? param_map["y4"] : 0.5f; 

            std::vector<cv::Point> contours(4);//四点定位墙

            contours[0].x=static_cast<int>(x1);
            contours[0].y=static_cast<int>(y1);
            contours[1].x=static_cast<int>(x2);
            contours[1].y=static_cast<int>(y2);
            contours[2].x=static_cast<int>(x3);
            contours[2].y=static_cast<int>(y3);
            contours[3].x=static_cast<int>(x4);
            contours[3].y=static_cast<int>(y4);

    

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();


            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
            float little_target_conf_thres  = param_map.count("little_target_conf_thres") ? param_map["little_target_conf_thres"] : 0.2f;

            empty_map_abi.add_or_update("conf_thres",con_thres);
            empty_map_abi.add_or_update("nms_thres", nms_thres);
            empty_map_abi.add_or_update("little_target_conf_thres",little_target_conf_thres);


            // exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, empty_map_abi);
            
            std::vector<std::vector<float>> nms_result;

            std::vector<PostureInfo> person_infos; 
            std::vector<climb::box_info_internal> boxs;
            for (auto pinfo : posture_info_list) 
            {
                PostureInfo person_info(pinfo); 
                person_infos.push_back(pinfo);
                std::vector<float> temp(4);
                temp[0]=person_info.x1;
                temp[1]=person_info.y1;
                temp[2]=person_info.x2;
                temp[3]=person_info.y2;
                temp[4]=person_info.score;

    // for (size_t i = 0; i < person_info.Kpoints.size(); i++)
    // {   
    //     cv::circle(image,  cv::Point(int( person_info.Kpoints[i].first.x ), int(person_info.Kpoints[i].first.y ) ), 3, cv::Scalar(0, 0, 255));
    // }

                nms_result.push_back(temp);

                climb::box_info_internal box;
                box.x1=person_info.x1;
                box.y1=person_info.y1;
                box.x2=person_info.x2;
                box.y2=person_info.y2;
                box.confidence=person_info.score;
                boxs.push_back(box);
            }

            // cv::imwrite("../plypoint.jpg",image);

            auto in_region_labels = in_region(nms_result, contours  );

            auto results = exposing::make_param_vector<climb::box_info>();

            for(int i=0;i<in_region_labels.size();i++) 
            {
                // std::cout<<in_region_labels[i]<<"  "<<person_infos[i].is_climb_posture()<<std::endl;
                boxs[i].category = in_region_labels[i] * person_infos[i].is_climb_posture() ;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(boxs[i]));
            }

            return results;
        }

        std::string version()
		{
			const std::string algo_module_version = "1.2.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

            // exposing::param_string nn_frame_version_param= posture_instance_.version();
#else
            // exposing::param_string nn_frame_version_param = posture_instance_.version();
#endif      
            exposing::param_string nn_frame_version_param ;
            std::string nn_frame_version =  exposing::to_narrow_string(nn_frame_version_param);
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:

        /**  @fun letterbox
         *   @param image scaleFill
         *   @return letterbox(image)
         *   @details Resize and pad image while meeting stride-multiple constrain
         */


    private:
    
        std::string model_directory_;
        int device_;
        // posture::detect_code posture_instance_;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<climb::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}

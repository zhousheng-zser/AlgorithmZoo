#include <iostream>
#include <cmath>
#include <tuple>

#include "../posture/detect_code.hpp"

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>
#include "general.hpp"
#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>
#define no_draw_pic 

namespace glasssix::smoke
{
    bool compareByFifthElement(const ObjectInfo& a, const ObjectInfo& b) {
                        return a.score > b.score;   }
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("smoke", false),  exposing::to_narrow_string(model_directory), device} 
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                net_smoke_detect_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/cigarette_detect.rknn", device);
#elif defined(USE_BMNN)
                net_smoke_detect_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/cigarette_detect.bmodel", device);
#endif      
            net_smoke_detect_->manual_possible_normalization(std::array<float,3>{0.f,0.f,0.f},std::array<float,3>{1.f / 255.f,1.f / 255.f,1.f / 255.f});
            yolov8_instance = std::make_shared<Yolov8<GenPipeline>>(320,320, net_smoke_detect_);
        }

        exposing::param_vector<smoke::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
                  
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                  throw exposing::abi_invalid_argument("incorrect roi in smoke");

            std::vector<smoke::box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            float smoke_conf_thres          = param_map.count("smoke_conf_thres") ? param_map["smoke_conf_thres"] : 0.7f;
            float smoke_iou_thres           = param_map.count("smoke_iou_thres") ? param_map["smoke_iou_thres"] : 0.65f;

            cv::Mat draw = image.clone();

            for (auto pinfo : posture_info_list) 
            {
                PostureInfo postureInfo{ pinfo };
                Smoke_Point smoke_point(postureInfo.x1,postureInfo.y1,postureInfo.x2,postureInfo.y2,postureInfo.score,postureInfo.Kpoints );
                safe_crop_rect detect_rect = smoke_point.get_upper_body_area(image.cols,image.rows);
                safe_crop_rect head_rect = smoke_point.get_head_area(image.cols,image.rows);

#ifdef draw_pic
                cv::rectangle(draw, cv::Point(detect_rect.x1, detect_rect.y1), cv::Point(detect_rect.x2, detect_rect.y2), cv::Scalar(0, 0, 255), 2);
                cv::rectangle(draw, cv::Point(head_rect.x1, head_rect.y1), cv::Point(head_rect.x2, head_rect.y2), cv::Scalar(255, 255, 0), 2);
                cv::circle(draw,  cv::Point(int( smoke_point.wrists[0].first.x ), int(smoke_point.wrists[0].first.y  ) ), 3, CV_RGB(0, 0,255), 3);  
                cv::circle(draw,  cv::Point(int( smoke_point.wrists[1].first.x ), int(smoke_point.wrists[1].first.y  ) ), 3, CV_RGB(0, 0,255), 3);  
                cv::imwrite("..//" + std::to_string(10)+".jpg",draw);
#endif // draw

                //获取头嘴框中心点到手腕最近距离
                if(!smoke_point.is_detect() || !head_rect.is_distance_of_centre_and_wrist_lessthan_detect_box_threhold( smoke_point.wrists, 
                                                                                                                        std::max(detect_rect.x2-detect_rect.x1,detect_rect.y2-detect_rect.y1) ))
                { continue;}  //    

                cv::Mat cigarette_detect = image(cv::Range(detect_rect.y1, detect_rect.y2), cv::Range(detect_rect.x1, detect_rect.x2));
               
                auto cigarette_objects = yolov8_instance->get_objects( cigarette_detect, smoke_conf_thres, smoke_iou_thres );

                Cigrate_box b(head_rect.x1,head_rect.y1,head_rect.x2,head_rect.y2) ;

                if(cigarette_objects.size()>0)
                {
                    std::sort( cigarette_objects.begin(), cigarette_objects.end(), compareByFifthElement   );

                    auto cigrate = cigarette_objects[0];
                    {
                        int cigratex1=std::round( cigrate.x1 +detect_rect.x1)>0?std::round( cigrate.x1 +detect_rect.x1):0  ;
                        int cigratey1=std::round( cigrate.y1 +detect_rect.y1)>0?std::round( cigrate.y1 +detect_rect.y1):0  ;
                        int cigratex2=std::round( cigrate.x2 +detect_rect.x1)<image.cols?std::round( cigrate.x2 +detect_rect.x1):image.cols ;
                        int cigratey2=std::round( cigrate.y2 +detect_rect.y1)<image.rows?std::round( cigrate.y2 +detect_rect.y1):image.rows ;
                    
                        Cigrate_box a(cigratex1,cigratey1,cigratex2,cigratey2);
    #ifdef draw_pic
                        cv::rectangle(draw, cv::Point(cigratex1, cigratey1), cv::Point(cigratex2, cigratey2), cv::Scalar(0, 255, 255), 2);
                        cv::imwrite("..//" + std::to_string(100)+".jpg",draw);
    #endif //draw
                        if(is_filterated( b,a ) )
                            continue;

                        float iou = IOU_compute(a, b);
                        smoke::box_info_internal temp_box;
                            temp_box.x1 = postureInfo.x1;
                            temp_box.x2 = postureInfo.x2;
                            temp_box.y1 = postureInfo.y1;
                            temp_box.y2 = postureInfo.y2;
                            temp_box.confidence = cigrate.score;
                            temp_box.key_points = exposing::make_param_vector<float>();
                            for(int j=0; j<postureInfo.Kpoints.size(); j++)
                            {
                                temp_box.key_points.push_back(postureInfo.Kpoints[j].first.x);
                                temp_box.key_points.push_back(postureInfo.Kpoints[j].first.y);
                                temp_box.key_points.push_back(postureInfo.Kpoints[j].second);
                            }
                            
                        if(iou>0.f)
                        {
                            temp_box.category = 0;
                            results.push_back(temp_box);
                        }
                        else
                        {
                            temp_box.category = 1;
                            results.push_back(temp_box);
                        }
                    }
                }
            }

            for (auto& box : results)
                result.push_back(exposing::make_as_first<box_info_impl>(box));

            return result;
        }

        
        std::string version()
        {
            const std::string algo_module_version = "3.0.2";
            std::string nn_frame_version = net_smoke_detect_->version();
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        std::shared_ptr<GenPipeline> net_smoke_detect_;
        std::shared_ptr<Yolov8<GenPipeline>> yolov8_instance;
        std::string model_directory_;
        exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
        int device_ ;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<smoke::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}

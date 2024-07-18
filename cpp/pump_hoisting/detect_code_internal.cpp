#include <iostream>
#include <cmath>
#include <tuple>
#include "../pedestrian/classify_code.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>
#include "general.hpp"

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

#define not_draw_pic 
#define LIBRARY_ID_MAX 1073741824
namespace glasssix::pump_hoisting
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{exposing::to_narrow_string(model_directory), device} 
        {
        }
        impl(std::string model_directory, int device) 
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string model_ext{ ".rknn" };
            
            const int letter_h = 736;
            const int letter_w = 1280;
#elif defined(USE_BMNN)
            std::string model_ext{ ".bmodel" };
            const int letter_h = 1280;
            const int letter_w = 1280;
#else
            std::string model_ext(".onnx");
            const int letter_h = 1280;
            const int letter_w = 1280;
#endif
            net_pump_hoisting_detect2_ = std::make_shared<GenPipeline>(model_directory + "/pump_hoisting" + model_ext, device);
            net_pump_hoisting_detect2_->manual_possible_normalization(0, 1.f / 255);
            yolov8_instance = std::make_shared<Yolov8<GenPipeline>>(letter_w,letter_h, net_pump_hoisting_detect2_);
        } 


        std::vector<Quadrilateral> get_dangerous_rect(std::vector<Rectangle>& all_pump_rect_boxes, cv::Mat& img ,int device_id =0, float move_threshold=0.05)
        {
            std::vector<Quadrilateral> dangerous_region;
            std::map<int, Rectangle> library;
            if( librarys.count(device_id) )
                library = librarys[device_id];
            if( library.size()==0 )
                first_init=1;
            if( first_init )
            {
                for(auto& current_box : all_pump_rect_boxes)
                {
                    id = (id+1) % LIBRARY_ID_MAX;
                    library[id] = current_box;
                }
                first_init = false;
            }
            else
            {
                for( auto& current_box : all_pump_rect_boxes )
                {
                    auto match_id = get_match_id(library, current_box,0.2);
                    if( match_id != -1 )  
                    {
                        float distance = get_distance_between_Rectangle(current_box, library[match_id], false );
                        float left_down_corner_distance = get_left_down_corner_distance_between_Rectangle(current_box,library[match_id],false);
                        float left_top_corner_distance =  get_left_top_corner_distance_between_Rectangle(current_box,library[match_id],false);
                        library[match_id].refresh( current_box.x1, current_box.y1, current_box.x2, current_box.y2   );
                        // if(  (abs(current_box.y2-current_box.y1)*0.08 > distance && distance > abs(current_box.y2-current_box.y1)*move_threshold) && left_down_corner_distance>0.08*abs(current_box.y2-current_box.y1)  && left_top_corner_distance>0.08*abs(current_box.y2-current_box.y1) ) //检测到移动了
                        if(  (abs(current_box.y2-current_box.y1)*0.8 > distance && distance > abs(current_box.y2-current_box.y1)*move_threshold) && left_down_corner_distance>0.08*abs(current_box.y2-current_box.y1)  && left_top_corner_distance>0.08*abs(current_box.y2-current_box.y1) ) //检测到移动了
                        {
                            auto neighboor = find_nearest_rectangles(all_pump_rect_boxes, current_box );
                            auto neighboorleft = std::get<0>(neighboor);
                            auto neighboorright = std::get<1>(neighboor);
                            auto neighboor_quadrilateral=get_initial_quadrilateral(neighboorleft,current_box ,neighboorright );
                            auto quadrilateral_left  = std::get<0>(  neighboor_quadrilateral);
                            auto quadrilateral_right = std::get<1>(  neighboor_quadrilateral);
                            if(!neighboorleft.is_invalid_rect() && calculate_distance_adjacent_edge(neighboorleft,current_box)<1000  )
                            {
                                Quadrilateral_scale(quadrilateral_left,0.8);
                                dangerous_region.push_back(quadrilateral_left);
                            }
                            if(!neighboorright.is_invalid_rect() && calculate_distance_adjacent_edge(neighboorright,current_box)<1000   )
                            {
                                Quadrilateral_scale(quadrilateral_right,0.8,false);
                                dangerous_region.push_back(quadrilateral_right);
                            }
                            int radius = 5;
                            // 绘制圆
#ifdef draw_pic
                            cv::circle(img, cv::Point(quadrilateral_left.x1,quadrilateral_left.y1), radius, cv::Scalar(0, 0, 255), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x2,quadrilateral_left.y2), radius, cv::Scalar(0, 0, 255), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x3,quadrilateral_left.y3), radius, cv::Scalar(0, 0, 255), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x4,quadrilateral_left.y4), radius, cv::Scalar(0, 0, 255), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x1,quadrilateral_right.y1), radius, cv::Scalar(0, 255, 125), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x2,quadrilateral_right.y2), radius, cv::Scalar(0, 255, 125), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x3,quadrilateral_right.y3), radius, cv::Scalar(0, 255, 125), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x4,quadrilateral_right.y4), radius, cv::Scalar(0, 255, 125), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x1,quadrilateral_left.y1), radius, cv::Scalar(255, 0, 25), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x2,quadrilateral_left.y2), radius, cv::Scalar(255, 0, 25), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x3,quadrilateral_left.y3), radius, cv::Scalar(255, 0, 25), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_left.x4,quadrilateral_left.y4), radius, cv::Scalar(255, 0, 25), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x1,quadrilateral_right.y1), radius, cv::Scalar(0, 255, 1), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x2,quadrilateral_right.y2), radius, cv::Scalar(0, 255, 1), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x3,quadrilateral_right.y3), radius, cv::Scalar(0, 255, 1), 2); // 参数：图像，圆心，半径，颜色，线宽
                            cv::circle(img, cv::Point(quadrilateral_right.x4,quadrilateral_right.y4), radius, cv::Scalar(0, 255, 1), 2); // 参数：图像，圆心，半径，颜色，线宽 
#endif                             
                        }
                    }
                    else    // 新添加特征进入特征库
                    {
                        library[(id++) % LIBRARY_ID_MAX] = current_box;
                    }
                }
            }
            librarys[device_id] = library;
            return dangerous_region;
        }
     
        exposing::param_vector<pump_hoisting::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,  std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                  throw exposing::abi_invalid_argument("incorrect roi in pump_hoisting");
            auto result = exposing::make_param_vector<box_info>();
            auto  empty_map_abi             = exposing::make_param_hash_map<exposing::param_string, float>();
            float conf_threshold            = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_threshold             = param_map.count("iou_threshold") ? param_map["iou_threshold"] : 0.7f;
            float move_threshold            = param_map.count("move_threshold") ? param_map["move_threshold"] : 0.05f;
            int   device_id                 = std::round( param_map.count("device_id") ? param_map["device_id"] : 0.f );
            float left_top_move_thres            = param_map.count("left_top_move_thres") ? param_map["left_top_move_thres"] : 0.08f;
            float left_down_move_thres            = param_map.count("left_down_move_thres") ? param_map["left_down_move_thres"] : 0.08f;
            float centre_move_thres            = param_map.count("centre_move_thres") ? param_map["centre_move_thres"] : 0.05f;
            empty_map_abi.add_or_update("conf_thres",conf_threshold) ;
            empty_map_abi.add_or_update("nms_thres", 0.45);
            cv::Mat draw = image.clone();

            // 获取检测到的对象
            auto pump_objects =  yolov8_instance->get_objects( draw);

            std::vector<Rectangle>  all_current_boxes;

            for(auto& var : pump_objects)//x1,y1,x2,y2
            {
                Rectangle temp(var.x1,var.x2,var.y1,var.y2 ,var.score );
                temp.refresh();
                if(( var.x2 -var.x1)>150)
                    all_current_boxes.push_back(temp);
            }
#ifdef draw_pic
            for(auto& current_box : all_current_boxes)
            {
                cv::rectangle(image, cv::Point(current_box.x1,current_box.y1),  
                                cv::Point(current_box.x2,current_box.y2),  cv::Scalar(0,0,255) ,5);
            }
            cv::imwrite( "../pumprect.jpg", image );
#endif
            auto dangerous_regions = get_dangerous_rect(all_current_boxes, image, device_id, move_threshold) ;
            if( dangerous_regions.size())
            {
                time_t tmep_Sec;
                time(&tmep_Sec);
                if(time_register.count(device_id)) // 有对应的表
                {
                    bool first_init =  time_register[device_id].first_init;
                    if(first_init) //第一次初始化
                    {
                        time_register[device_id].first_init = false;
                        time_register[device_id].first_alarm_time = tmep_Sec; 
                    }  
                    else   //并非第一次初始化  判断时间是否大于时间间隔
                    {
                        if( abs(tmep_Sec - time_register[device_id].first_alarm_time)>40 )//大于间隔 初始化标志位 并且删除相应库信息
                        {
                            //set time sign invalidity
                            time_register[device_id].first_init = true;
                            remove_library_by_id(device_id);
                        }
                        else{}
                    }
                }
                else{                        //无对应的表
                    time_sign temp_time_sign;
                    temp_time_sign.first_init = false;
                    temp_time_sign.first_alarm_time = tmep_Sec; 
                    time_register[device_id] = temp_time_sign;
                }
            }
            for (auto& dangerous_region : dangerous_regions)
            {
                pump_hoisting::box_info_internal temp_box{dangerous_region.x1,dangerous_region.y1,dangerous_region.x2,dangerous_region.y2,
                                                         dangerous_region.x3,dangerous_region.y3,dangerous_region.x4,dangerous_region.y4,0,0 };
                result.push_back(exposing::make_as_first<box_info_impl>(temp_box));
            }  
            return result;
        }
        std::string remove_library()
        {
            librarys.clear();
            first_init=true;
            id=0;
            const std::string delete_library = "ok";
            return delete_library;
        }
        std::string remove_library_by_id(int device_id)
        {
            std::map<int, Rectangle> library;
            librarys[device_id]=library;
            const std::string delete_library = "ok";
            return delete_library;
        }
        std::string version()
        {
            const std::string algo_module_version = "3.0.2";   
            std::string nn_frame_version = "dsdsd";
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }
  
    private:
        // GenPipeline*  net_pump_hoisting_detect2_;
        std::shared_ptr<GenPipeline> net_pump_hoisting_detect2_;
        std::shared_ptr<Yolov8<GenPipeline, false>> yolov8_instance;

        pedestrian::classify_code pedestrain_instance_;
        std::string model_directory_;
        static std::map<int, std::map<int, Rectangle>>  librarys;
        static std::map<int, time_sign> time_register; //device_id
        static bool first_init ;
        static int id ;
        exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
        std::vector<int> add_weight;
        std::vector<int> mul_weight;
        int device_ ;
    };
    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }
    detect_code_internal::~detect_code_internal() = default;
    exposing::param_vector<pump_hoisting::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,  std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
    std::string detect_code_internal::version()
	{
		return impl_->version();
	}
    std::string detect_code_internal::remove_library()
	{
		return impl_->remove_library();
	}
    std::map<int, std::map<int, Rectangle>> detect_code_internal::impl::librarys;
    std::map<int, time_sign> detect_code_internal::impl::time_register;
    bool detect_code_internal::impl::first_init = true;
    int  detect_code_internal::impl::id =0 ;
}

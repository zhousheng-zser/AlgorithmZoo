#include <iostream>
#include <cmath>
#include <tuple>
#include "../pedestrian/classify_code.hpp"
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
#define draw_pic 
#define LIBRARY_ID_MAX 1073741824
namespace glasssix::pump_hoisting
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{pump_hoisting::get_model_params("pump_hoisting", false),  exposing::to_narrow_string(model_directory), device} 
        {
        }
        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :net_pump_hoisting_detect_(phai,  model_directory + std::string("/pump.rknn"), device), model_directory_(model_directory)
        {
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("pedestrian");
            pedestrain_instance_ = glasssix::exposing::make_exported_interface<pedestrian::classify_code>(exposing::param_string(model_directory), device);
            model640 = false;//1280<->false 640 <->true
            init_data_compatible(1280,736);
        } 
        std::vector<Rectangle> get_pumprect(cv::Mat& image, float conf_thres=0.6, float iou_thres=0.7)
        {
            std::vector<Rectangle> Out_xy;
            // auto new_shape = cv::Size(model640? 640:1280, model640? 640:1280);
            auto new_shape = cv::Size(1280, 736);
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image, pad_h, pad_w, new_shape ) ;
            auto  network_results = net_pump_hoisting_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
            std::vector<std::string>  out_names={"355","340","output0"};
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
            for (size_t i=0;i< out_names.size(); i++) //对输出数据做处理
                forwards.push_back(network_results[out_names[i]]);
            int num =0;
            auto real_output = Yovo8se_Concat(forwards,0.1,num,1280,736);//5*8400
            auto post_result = post_process(real_output,pad_h,pad_w, 1.f/ratio, num, conf_thres, iou_thres );
            for(auto& var : post_result)//x1,y1,x2,y2
            {
                Rectangle temp(var[0],var[2],var[1],var[3] ,var[4] );
                temp.refresh();
                if(( var[2] -var[0])>150)
                    Out_xy.push_back(temp);
            }
            return Out_xy;
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
                        // float distance1 =get_distance_between_Rectangle(current_box, library[match_id] );
                        float distance = get_distance_between_Rectangle(current_box, library[match_id], false );
                        float left_down_corner_distance = get_left_down_corner_distance_between_Rectangle(current_box,library[match_id],false);
                        float left_top_corner_distance = get_left_top_corner_distance_between_Rectangle(current_box,library[match_id],false);
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
                            if(!neighboorleft.is_invalid_rect() )
                            {
                                Quadrilateral_scale(quadrilateral_left,0.8);
                                dangerous_region.push_back(quadrilateral_left);
                            }
                            if(!neighboorright.is_invalid_rect() )
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
        std::shared_ptr<memory::tensor<float>> Yovo8se_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf,int& candicate_num,int width = 1280, int height =1280 )
        {
            conf = de_sigmoid(conf);
            int input = width*height;
            int box_tmp_size = 64;
            int stride_8_num = input / 64;
            int stride_16_num = input / 256;
            int stride_32_num = input / 1024;
            int candidate_num = stride_8_num + stride_16_num + stride_32_num ;
            int totol_size = stride_8_num + stride_16_num + stride_32_num ;    
            //20 40 80 
            const float *data_stride_8 = outs[2]->cpu_data();
            const float *data_stride_16 = outs[1]->cpu_data();
            const float *data_stride_32 = outs[0]->cpu_data();
            std::vector<int> match_index;
            const float* data_stride_8_conf = data_stride_8+stride_8_num*box_tmp_size;
            for (size_t i = 0; i < stride_8_num; i++)
                if( data_stride_8_conf[i] >conf  )
                    match_index.push_back(i);
            const float* data_stride_16_conf = data_stride_16+stride_16_num*box_tmp_size;
            for (size_t i = 0; i < stride_16_num; i++)
                if( data_stride_16_conf[i]>conf )
                    match_index.push_back(i+stride_8_num);
            const float* data_stride_32_conf = data_stride_32+stride_32_num*box_tmp_size;
            for (size_t i = 0; i < stride_32_num; i++)
                if( data_stride_32_conf[i] >conf  )    
                    match_index.push_back(i+ stride_8_num + stride_16_num );
            //concat the 80*40 40*40 20*20 
            std::vector<float> cat(65*candidate_num); //1*65*candidate_num = 64*candidate_num + 1*candidate_num        
            for(int i=0,j=0;i<65;i++,j=0)
            {   
                std::copy(data_stride_8+i*stride_8_num, data_stride_8+(i+1)*stride_8_num, cat.data()+i*candidate_num ); 
                std::copy(data_stride_16+i*stride_16_num, data_stride_16+(i+1)*stride_16_num, cat.data()+i*candidate_num+stride_8_num ); 
                std::copy(data_stride_32+i*stride_32_num, data_stride_32+(i+1)*stride_32_num, cat.data()+i*candidate_num+stride_8_num+stride_16_num ); 
            }
            //tranpose and softmax
            std::vector<float> reshape_box(candidate_num*64);
            tranpose(cat.data(),reshape_box.data(),64,candidate_num );
            candidate_num = match_index.size();
            candicate_num = candidate_num;
            std::vector<float> reshape_boxtmp(candidate_num*64);
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));
            for (size_t i = 0; i < match_index.size(); i++)
                std::copy(reshape_box.data()+match_index[i]*64,reshape_box.data()+match_index[i]*64+64,reshape_boxtmp.data()+i*64);
            int index = 0;
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    Softmax(reshape_boxtmp.data()+ 16*index++ ,16 ) ;//inplace softamax
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    for(int k=0; k<16; k++)
                        cat[k*4*candidate_num +j*candidate_num +i ] = reshape_boxtmp[i*16*4 + j*16+k ];
            //16 channels 1*1convolution
            std::vector<float> conv(4*candidate_num,0);
            for(int i=0;i<16;i++)
                for(int j=0;j<4*candidate_num;j++)
                    conv[j] = conv[j] +cat[i*4*candidate_num+j ]*i  ; 
            std::vector<float>  concat(candidate_num*4);
            for(int i=0;i<candidate_num*2;i++)
            {              
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + add_weight[ i<candidate_num? match_index[i]:(match_index[i - candidate_num ]+totol_size) ] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );      // add_data[i]-sub_data[i]) ;  
            }
            //concat the output
            float * output = output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                output[candidate_num*0 +i] = concat[candidate_num*0 +i]*mul_weight[ match_index[i]];    
                output[candidate_num*1 +i] = concat[candidate_num*1 +i]*mul_weight[ match_index[i]];
                output[candidate_num*2 +i] = concat[candidate_num*2 +i]*mul_weight[ match_index[i]];
                output[candidate_num*3 +i] = concat[candidate_num*3 +i]*mul_weight[ match_index[i]];
                output[candidate_num*4 +i] =  sigmoid_x(cat[totol_size*64 +match_index[i]]);
            }          
            return  output0;
        }
        exposing::param_vector<pump_hoisting::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,  std::map<std::string, float>& param_map)
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
            std::vector<int> pedestrain_list;
            auto all_current_boxes = get_pumprect(image,conf_threshold);
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
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
            std::string nn_frame_version = net_pump_hoisting_detect_.version();
#else
            std::string nn_frame_version = net_pump_hoisting_detect_.version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }
    private:
       void init_data_compatible(int width,int height)
        {
            int size_mul_weight = width*height*21/1024; 
            int size_add_weight = 2*size_mul_weight;  
            int width_base = width/8;
            int height_base = height/8;
            int candicate_area = width_base*height_base;
            add_weight.resize(size_add_weight);
            mul_weight.resize(size_mul_weight);
            for (size_t i = 0; i < candicate_area*21/16; i++)
            {     
                if(i< candicate_area  ) 
                {
                    add_weight[i] = i%(width_base); 
                    add_weight[i+size_mul_weight] = i/(width_base) ; 
                    mul_weight[i] =8.f;
                }
                else if( i<int(std::round(i - candicate_area*1.25)))
                {
                    add_weight[i] = (i -candicate_area)% (width_base/2);
                    add_weight[i+size_mul_weight] = (i-candicate_area)/ (width_base/2);
                    mul_weight[i] = 16.f;
                }
                else
                {
                    add_weight[i] = int(std::round(i - candicate_area*1.25)) % (width_base/4);
                    add_weight[i+size_mul_weight] =  int(std::round(i - candicate_area*1.25))/(width_base/4);
                    mul_weight[i] = 32.f;
                }
            }
            return ;    
        }
    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		rknnwrapper::rknn_wrapper net_pump_hoisting_detect_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_pump_hoisting_detect_;
#endif
        pedestrian::classify_code pedestrain_instance_;
        std::string model_directory_;
        bool model640 = true;
        static std::map<int, std::map<int, Rectangle>>  librarys;
        static std::map<int, time_sign> time_register; //device_id
        static bool first_init ;
        static int id ;
        exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
        std::vector<float> add_weight;
        std::vector<float> mul_weight;
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

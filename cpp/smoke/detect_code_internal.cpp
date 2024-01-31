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

namespace glasssix::smoke
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("smoke", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :net_smoke_detect_(phai,  model_directory + std::string("/cigarette_detect.rknn"), device), model_directory_(model_directory)
        {
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            // posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device,1);
            init_data();
        } 

        exposing::param_vector<smoke::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in smoke");
            }

            std::vector<smoke::box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            auto  empty_map_abi             = exposing::make_param_hash_map<exposing::param_string, float>();
            float conf_threshold            = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float smoke_conf_thres          = param_map.count("smoke_conf_thres") ? param_map["smoke_conf_thres"] : 0.45f;

            empty_map_abi.add_or_update("conf_thres",conf_threshold);
            empty_map_abi.add_or_update("nms_thres", 0.45);

            // exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, empty_map_abi);
            
            cv::Mat draw = image.clone();

            int indexxx=0;
            std::cout<<posture_info_list.size()<<std::endl;
            for (auto pinfo : posture_info_list) 
            {
                PostureInfo postureInfo{ pinfo };
                int color_index =0;
                for(auto var : postureInfo.Kpoints)
                {
                        // cv::circle(draw,  cv::Point(int( var.first.x ), int(var.first.y  ) ), 3, CV_RGB(0, 0,255), 3);  
                }

                Smoke_Point smoke_point(postureInfo.x1,postureInfo.y1,postureInfo.x2,postureInfo.y2,postureInfo.score,postureInfo.Kpoints );
                safe_crop_rect detect_rect = smoke_point.get_upper_body_area(image.cols,image.rows);
                safe_crop_rect head_rect = smoke_point.get_head_area(image.cols,image.rows);
                // cv::rectangle(draw, cv::Point(detect_rect.x1, detect_rect.y1), cv::Point(detect_rect.x2, detect_rect.y2), cv::Scalar(0, 0, 255), 2);
                // cv::rectangle(draw, cv::Point(head_rect.x1, head_rect.y1), cv::Point(head_rect.x2, head_rect.y2), cv::Scalar(255, 255, 0), 2);

                // cv::circle(draw,  cv::Point(int( smoke_point.wrists[0].first.x ), int(smoke_point.wrists[0].first.y  ) ), 3, CV_RGB(0, 0,255), 3);  

                // cv::circle(draw,  cv::Point(int( smoke_point.wrists[1].first.x ), int(smoke_point.wrists[1].first.y  ) ), 3, CV_RGB(0, 0,255), 3);  

                // cv::imwrite("..//" + std::to_string(10)+".jpg",draw);
                if(!smoke_point.is_detect() || head_rect.is_distance_between_centre_wrist_less_detect_box_threhold( smoke_point.wrists,std::max(detect_rect.x2-detect_rect.x1,detect_rect.y2-detect_rect.y1)) )
                    continue;

                cv::Mat cigarette_detect = image(cv::Range(detect_rect.y1, detect_rect.y2), cv::Range(detect_rect.x1, detect_rect.x2));
               
                auto smoke_detect_shape = cv::Size(320,  320);

                // cv::rectangle(draw, cv::Point(cigarette_detect.x1, cigarette_detect.y1), cv::Point(cigarette_detect.x2, cigarette_detect.y2), cv::Scalar(0, 0, 255), 2);
              

                cv::Mat cigarette_detect_blob;
                float smoke_ratio = 0;
                int smoke_pad_h=0;  
                int smoke_pad_w=0;

                std::tie(cigarette_detect_blob, smoke_ratio) = preprocess_detection( cigarette_detect,smoke_pad_h,smoke_pad_w, smoke_detect_shape ) ;
                auto smoke_net_result = net_smoke_detect_.forward(cigarette_detect_blob.data, { 1, cigarette_detect_blob.rows, cigarette_detect_blob.cols,cigarette_detect_blob.channels() }, RKNN_TENSOR_NHWC);
                std::vector<std::string>  somke_out_names={"440","425","410","output0"};

                std::vector<std::shared_ptr<memory::tensor<float>>> smoke_forwards;
                for (size_t i=0;i< somke_out_names.size(); i++)
                    smoke_forwards.push_back(smoke_net_result[somke_out_names[i]]);
                
                int smoke_candicate_num=0;
                auto smoke_output = Yovo8se_Concat_4B(smoke_forwards,smoke_conf_thres,smoke_candicate_num,posture_add_weight,posture_mul_weight);//5*8400
                auto nms_results = smoke_post_process(smoke_output, smoke_pad_h,smoke_pad_w, 1.f/smoke_ratio,smoke_candicate_num);
                //  std::cout<<"nms_results: "<<nms_results.size()<<" "<<std::endl;
                Cigrate_box b(head_rect.x1,head_rect.y1,head_rect.x2,head_rect.y2) ;


                for(auto& cigrate:nms_results)
                {
                    int cigratex1=std::round( cigrate[0] +detect_rect.x1)>0?std::round( cigrate[0] +detect_rect.x1):0  ;
                    int cigratey1=std::round( cigrate[1] +detect_rect.y1)>0?std::round( cigrate[1] +detect_rect.y1):0  ;
                    int cigratex2=std::round( cigrate[2] +detect_rect.x1)<image.cols?std::round( cigrate[2] +detect_rect.x1):image.cols ;
                    int cigratey2=std::round( cigrate[3] +detect_rect.y1)<image.rows?std::round( cigrate[3] +detect_rect.y1):image.rows ;
                 
                    Cigrate_box a(cigratex1,cigratey1,cigratex2,cigratey2);

                    if(is_filterated( b,a ) )
                    {
                        continue;
                    }

                    float iou = IOU_compute(a, b);
                    smoke::box_info_internal temp_box;

                        temp_box.x1 = postureInfo.x1;
                        temp_box.x2 = postureInfo.x2;
                        temp_box.y1 = postureInfo.y1;
                        temp_box.y2 = postureInfo.y2;
                        temp_box.confidence = postureInfo.score;

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
                indexxx++;
            }

            for (auto& box : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(box));
            }   
            return result;
        }

        
        std::string version()
        {
            const std::string algo_module_version = "3.0.3";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
            std::string nn_frame_version = net_smoke_detect_.version();
#else
            std::string nn_frame_version = net_smoke_detect_.version();
#endif
        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
      
        /**
         * @fun post_process
         * @param outs, conf_thres, iou_thres
         * @return nms_bbox
         */
      

        /**
         * @fun run_detect
         * @param image, param_map
         * @return bbox
         */


        void init_data()
        {
            int stride_sum = 8500;
            int stride_4 = 80*80; 
            int stride_8 = 40*40;
            int stride_16 = 20*20;
            int stride_32 = 10*10;
            posture_add_weight.resize(stride_sum*2);
            posture_mul_weight.resize(stride_sum);
            for(int i=0;i<stride_sum;i++)
            {
                if( i<stride_4)
                {
                    posture_add_weight[i]=i%80;
                    posture_add_weight[i+stride_sum]=i/80;
                    posture_mul_weight[i]=4.f;
                }
                else if(i<(stride_4+stride_8))
                {
                    posture_add_weight[i]=(i -stride_4)%40;
                    posture_add_weight[i+stride_sum]= (i-stride_4)/40;
                    posture_mul_weight[i]=8.f;
                }
                else if(i<(stride_4+stride_8 + stride_16))
                {
                    posture_add_weight[i]=(i -stride_4-stride_8)%20;
                    posture_add_weight[i+stride_sum]=(i-stride_4-stride_8)/20;
                    posture_mul_weight[i]=16.f;
                }
                else
                {
                    posture_add_weight[i]=(i -stride_4-stride_8-stride_16)%10;
                    posture_add_weight[i+stride_sum]=(i - stride_4-stride_8-stride_16)/10;
                    posture_mul_weight[i]=32.f;
                }
            }
        }


    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_smoke_detect_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_smoke_detect_;
#endif
        std::string model_directory_;
        // posture::detect_code posture_instance_;
        exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;
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

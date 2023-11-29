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
            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device,1);
            init_data();
        } 

        exposing::param_vector<smoke::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
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

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();

            empty_map_abi.add_or_update("conf_thres", 0.4);
            empty_map_abi.add_or_update("nms_thres", 0.45);


            exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, empty_map_abi);
            
            std::vector<PostureInfo> persons_info;

            int indexxx=0;
            for (auto pinfo : posture_info_list) 
            {
                PostureInfo postureInfo{ pinfo };

                Smoke_Point smoke_point(postureInfo.x1,postureInfo.y1,postureInfo.x2,postureInfo.y2,postureInfo.score,postureInfo.Kpoints );

                float detect_x1 = 0.f;
                float detect_x2 = 0.f;
                float detect_y1 = 0.f;
                float detect_y2 = 0.f;
               
                std::tie(detect_x1,detect_x2,detect_y1,detect_y2) = smoke_point.get_upper_body_area();
                
                        // cv::rectangle(image, cv::Point(detect_x1, detect_y1), cv::Point(detect_x2, detect_y2), cv::Scalar(0, 0, 255), 2);

                float head_x1 = 0.f;
                float head_x2 = 0.f;
                float head_y1 = 0.f;
                float head_y2 = 0.f;
                std::tie(head_x1,head_x2,head_y1,head_y2) = smoke_point.get_head_area();

            
                if(! smoke_point.is_detect())
                    continue;

                // if(indexxx==0)
                // {
                //     cv::imwrite("../imagetetet.jpg",image);
                // }

                // cv::rectangle(image, cv::Point(head_x1, head_y1), cv::Point(head_x2, head_y2), cv::Scalar(255, 255, 0), 2);

                cv::Mat cigarette_detect = image(cv::Range(detect_y1, detect_y2), cv::Range(detect_x1, detect_x2));
                auto smoke_detect_shape = cv::Size(640,  640);
                
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
                auto smoke_output = Yovo8se_Concat_4B(smoke_forwards,0.45f,smoke_candicate_num,posture_add_weight,posture_mul_weight);//5*8400
                auto nms_results = smoke_post_process(smoke_output, smoke_pad_h,smoke_pad_w, 1.f/smoke_ratio,smoke_candicate_num);
                Cigrate_box b(head_x1,head_y1,head_x2,head_y2) ;

                for(auto& cigrate:nms_results)
                {
                    int cigratex1=std::round( cigrate[0] +detect_x1)>0?std::round( cigrate[0] +detect_x1):0  ;
                    int cigratey1=std::round( cigrate[1] +detect_y1)>0?std::round( cigrate[1] +detect_y1):0  ;
                    int cigratex2=std::round( cigrate[2] +detect_x1)<image.cols?std::round( cigrate[2] +detect_x1):image.cols ;
                    int cigratey2=std::round( cigrate[3] +detect_y1)<image.rows?std::round( cigrate[3] +detect_y1):image.rows ;

                    // if(indexxx==0)
                    // { 
                        // cv::rectangle(image, cv::Point(cigratex1, cigratey1), cv::Point(cigratex2, cigratey2), cv::Scalar(0, 255, 0), 2);
                    // }
                    Cigrate_box a(cigratex1,cigratey1,cigratex2,cigratey2);
                    float iou = IOU_compute(a, b);
                    if(iou>0.f)
                    {
                        smoke::box_info_internal temp_box;
                        temp_box.x1 = cigratex1;
                        temp_box.x2 = cigratex2;
                        temp_box.y1 = cigratey1;
                        temp_box.y2 = cigratey2;
                        temp_box.confidence = cigrate[4];
                        temp_box.category = 0;
                        results.push_back(temp_box);
                    }
                    else
                    {
                        smoke::box_info_internal temp_box;
                        temp_box.x1 = cigratex1;
                        temp_box.x2 = cigratex2;
                        temp_box.y1 = cigratey1;
                        temp_box.y2 = cigratey2;
                        temp_box.confidence = cigrate[4];
                        temp_box.category = 1;
                        results.push_back(temp_box);
                    }
                }
                indexxx++;
                cv::imwrite("../smoke.jpg",image);
            }


            for (auto& box : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(box));
            }   
            return result;
        }

        
        std::string version()
        {
            const std::string algo_module_version = "1.0.0";

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
         * @fun reset
         * @param x, size
         * @return reset(x) into cv::Size
        */
        int reset(float x, int size)
        {
            if(x < 0)
                return 0;
            else if (x > size)
                return x;
            else 
                return static_cast<int>(x);
        }

        /**
         * @fun run_detect
         * @param image, param_map
         * @return bbox
         */
        std::vector<std::array<float,5>> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        {
            
            // float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.75f;
            // float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      

            // // preprocess
            // auto input_shape = cv::Size(640,  640);

            // auto output_shape = cv::Size(image.cols, image.rows);
            
            // cv::Mat blobs;
            // float ratio = 0;
            // std::tie (blobs, ratio) = letterbox(image, 640);

            // cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

            // std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            // std::vector<std::string>  phais;

            // auto  network_results = cigarette_detect_instance_.forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);

            // forwards.push_back(network_results["output0"]);
            // forwards.push_back(network_results["340"]);
            // forwards.push_back(network_results["355"]);

            // auto concat_output = concat(forwards, conf_threshold);

            // if(concat_output.empty())
            // {
                return  std::vector<std::array<float,5>>();
            // } 
            // else
            // {
            //     // post_process
            //     auto nms_result = post_process(concat_output, conf_threshold, iou_threshold);

            //     // scale_coords
            //     std::vector<std::array<float, 5>> detect_result;

            //     // for(auto &it: nms_result)
            //     // {
            //     //     std::array<float, 5> box_info;

            //     //     auto scale_coords = scale_coord(it, input_shape, output_shape);

            //     //     box_info[0] = reset(scale_coords[0], image.cols); 
            //     //     box_info[1] = reset(scale_coords[1], image.rows); 
            //     //     box_info[2] = reset(scale_coords[2], image.cols);
            //     //     box_info[3] = reset(scale_coords[3], image.rows);
            //     //     box_info[4] = scale_coords[4];

            //     //     detect_result.push_back(box_info);
            //     // }
                
            //     return detect_result;
            // }
        }
        
        /**
         * @fun is_rect_cross
         * @param box1_x1, box1_y1, box1_x2, box1_y2, box2_x1, box2_y1, box2_x2, box2_y2, 
         * 
         */
        bool is_rect_cross(int box1_x1, int box1_y1, int box1_x2, int box1_y2, int box2_x1, int box2_y1, int box2_x2, int box2_y2) {
            // 判断矩形是否相交
            if (std::max(box1_x1, box2_x1) > std::min(box1_x2, box2_x2) || std::max(box1_y1, box2_y1) > std::min(box1_y2, box2_y2))
                return false;
            else
                return true;
        }


        void init_data()
        {
            posture_add_weight.resize(34000*2);
            posture_mul_weight.resize(34000);
            for(int i=0;i<34000;i++)
            {
                if( i<25600)
                {
                    posture_add_weight[i]=i%160;
                    posture_add_weight[i+34000]=i/160;
                    posture_mul_weight[i]=4.f;
                }
                else if(i<25600+6400)
                {
                    posture_add_weight[i]=(i -25600)%80;
                    posture_add_weight[i+34000]= (i-25600)/80;
                    posture_mul_weight[i]=8.f;
                }
                else if(i<25600+8000)
                {
                    posture_add_weight[i]=(i -32000)%40;
                    posture_add_weight[i+34000]=(i-32000)/40;
                    posture_mul_weight[i]=16.f;
                }
                else
                {
                    posture_add_weight[i]=(i -33600)%20;
                    posture_add_weight[i+34000]=(i-33600)/20;
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
        posture::detect_code posture_instance_;
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


    exposing::param_vector<smoke::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}

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


#include "general.hpp"
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>

namespace glasssix::posture
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device, int model_type)
            : model_directory_{ std::string(model_directory) }, device_{ device },model_type_{model_type}
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
           
           if(model_type_==1)
           {              
                net_detect640_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture640_17.rknn", device);     
                net_detect1280_single_branch_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),  
                    std::string(model_directory) + "/" +"posture1280_17.rknn", device);  
           }
           else
           {
                net_detect640_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture12.rknn", device);     

                net_detect1280_single_branch_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),  
                    std::string(model_directory) + "/" +"posture1280_12.rknn", device); 
           }
           
#else
            if(model_type_==1)
            {
                net_detect640_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture640_17.racy", device);   
                net_detect1280_single_branch_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture1280_17.racy", device);      

            }
            else
            {
                net_detect640_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture12.racy", device); 
                net_detect1280_single_branch_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture1280_12.racy", device);   
            }
    #endif
            init_data();
        }

std::string version()
        {
			const std::string algo_module_version = "3.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string nn_frame_version = net_detect640_->version();
#else
			std::string nn_frame_version = net_detect640_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

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

            auto new_shape640 = cv::Size(640,  640);
            auto new_shape1280 = cv::Size(1280,  1280);

            cv::Mat blob640;
            cv::Mat blob1280;
            float ratio640 = 0;
            float ratio1280 = 0;
            int pad_h640=0;  
            int pad_w640=0;
            int pad_h1280=0;  
            int pad_w1280=0;

            std::tie(blob640, ratio640) = preprocess_detection( cropped_image,pad_h640, pad_w640, new_shape640 ) ;
            std::tie(blob1280, ratio1280) = preprocess_detection( cropped_image,pad_h1280, pad_w1280, new_shape1280 ) ;

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards640;
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards1280;

            auto network_result = net_detect640_->forward(blob640.data, { 1, blob640.rows, blob640.cols,blob640.channels() }, RKNN_TENSOR_NHWC);
            auto network_result_1280_single = net_detect1280_single_branch_->forward(blob1280.data, { 1, blob1280.rows, blob1280.cols,blob1280.channels() }, RKNN_TENSOR_NHWC);
           
            for (size_t i=0;i< out_names640.size(); i++)
                forwards640.push_back(network_result[out_names640[i]]);

            for (size_t i=0;i< out_names1280.size(); i++)
                forwards1280.push_back(network_result_1280_single[out_names1280[i]]);

            int candicate_num640=0;
            int candicate_num1280=0;

            auto real_output640 = Posture_Concat640(forwards640, keypoint_num,con_thres,candicate_num640,posture_add_weight,posture_mul_weight);
            auto real_output1280 = Posture_Concat1280(forwards1280, keypoint_num,little_target_conf_thres,candicate_num1280,posture_add_weight_1280single,posture_mul_weight_1280single);

            auto nms_input640  = XYXY2WH(real_output640, image,pad_h640,pad_w640, 1.f/ratio640,keypoint_num,candicate_num640);
            auto nms_input1280 = XYXY2WH(real_output1280, image,pad_h1280,pad_w1280, 1.f/ratio1280,keypoint_num,candicate_num1280);

            // for(auto var : nms_input640)
            // {
            //     cv::rectangle(image,   cv::Point((int) var[0]  , (int)  var[1] ),
            //                     cv::Point((int) (var[0] + var[2]  ) , (int) (var[1] + var[3]) ),  cv::Scalar(0, 255, 255), 2);   
            //     for(int j=0;j<17;j++)
            //     {
            //         cv::circle(image,  cv::Point((int) var[3*j+5]  , (int) var[3*j+ 1 +5]  ), 2, cv::Scalar(0, 255, 255));
            //     }
            // }

            // for(auto var : nms_input1280)
            // {
            //     cv::rectangle(image,   cv::Point((int) var[0]  , (int)  var[1] ),
            //                     cv::Point((int) (var[0] + var[2]  ) , (int) (var[1] + var[3]) ),  cv::Scalar(255, 0, 255), 2);   
            //     for(int j=0;j<17;j++)
            //     {
            //         cv::circle(image,  cv::Point((int) var[3*j+5]  , (int) var[3*j+ 1 +5]  ), 2, cv::Scalar(255, 0, 255));
            //     }
            // }
            // cv::imwrite("../adsfsfsdpreocesdssds.jpg",image);


            std::vector<std::vector<float>> nms_input( nms_input640.size() + nms_input1280.size() );
            int index =0;
            for(auto& var : nms_input640)
                nms_input[index++] = var;
            for(auto& var : nms_input1280)
                nms_input[index++] = var;
            auto nms_result_index = nms_process(nms_input,con_thres,iou_thres);


            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result;

            for (auto& id : nms_result_index)
            {
                box_info_internal temp_result;

                temp_result.x1 = nms_input[id][0];//       body[0]+ roi_x;
                temp_result.y1 = nms_input[id][1]+ roi_y;
                temp_result.x2 = nms_input[id][0] + nms_input[id][2]+roi_x;
                temp_result.y2 = nms_input[id][1] + nms_input[id][3]+ roi_y;
                temp_result.score = nms_input[id][4];
                temp_result.key_points = exposing::make_param_vector<float>();
                for(int j=0;j<keypoint_num;j++)
                {
                    temp_result.key_points.push_back(nms_input[id][3*j+5] + roi_x);
                    temp_result.key_points.push_back(nms_input[id][3*j+5+1] + roi_y);
                    temp_result.key_points.push_back(nms_input[id][3*j+5+2]);
                }
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));

            return fin_result;
        }

    
    private:      
        
        void init_data()
        {
            std::vector<std::string>  out_name_12keypoint={"413","398","output0", "368" };
            std::vector<std::string>  out_name_17keypoint={"413","398","output0", "368" };

            std::vector<std::string>  out_name_12keypoint1280={"/model.22/Reshape_3_output_0","/model.22/Reshape_output_0" };
            std::vector<std::string>  out_name_17keypoint1280={"/model.22/Reshape_3_output_0","/model.22/Reshape_output_0" };

            if(model_type_==0)
            {
                out_names640 = out_name_12keypoint;
                out_names1280 = out_name_12keypoint1280;
                keypoint_num=12;
            }
            else
            {
                 out_names640 = out_name_17keypoint;
                 out_names1280 = out_name_17keypoint1280;
                 keypoint_num=17;
            }

            posture_add_weight.resize(8400*2);
            posture_mul_weight.resize(8400);
            posture_add_weight_1280single.resize(25600*2);
            posture_mul_weight_1280single.resize(25600);

            for(int i=0;i<25600;i++)
            {
                    posture_add_weight_1280single[i] = i%160;;
                    posture_add_weight_1280single[i+25600] = i/160;
                    posture_mul_weight_1280single[i] =8.f;
            }


            for(int i=0;i<8400;i++)
            {
                if( i<6400)
                {
                    posture_add_weight[i]=i%80;
                    posture_add_weight[i+8400]=i/80;
                    posture_mul_weight[i]=8.f;
                }
                else if(i<8000)
                {
                    posture_add_weight[i]=(i -6400)%40;
                    posture_add_weight[i+8400]=(i -6400)/40;
                    posture_mul_weight[i]=16.f;
                }
                else
                {
                    posture_add_weight[i]=(i -8000)%20;
                    posture_add_weight[i+8400]=(i -8000)/20;
                    posture_mul_weight[i]=32.f;
                }

               

            }
        }


        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h640, int pad_w640,         
                                                                        float scale, int key_point_num, float threshold=0.7,float iou_thres=0.6 )
        {
            std::vector<std::vector<float>> output;

            int shape =5+key_point_num*3;
            const int candidate_num=8400;
            std::shared_ptr<glasssix::memory::tensor<float>> dest 
                    (new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));

            //56*8400->8400*56
            tranpose( net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
            const float *dest_ptr = dest->cpu_data(); 

            // std::vector<Bbox> xywh_box; for nms
            std::vector<cv::Rect2d> xywh_boxes;
            std::vector<std::vector<float>> key_points;
            std::vector<float> scores;
            std::vector<int> indices_body;//候选框顺序

            for(int i=0;i<candidate_num;i++)
            {
                if(dest_ptr[shape*i+4]>threshold )
                {
                    indices_body.push_back(i);
                    cv::Rect2d boxwh;
                    boxwh.x      =  static_cast<double>(dest_ptr[shape*i] - dest_ptr[shape*i+2] / 2 );
                    boxwh.y      =  static_cast<double>(dest_ptr[shape*i+1] - dest_ptr[shape*i+3]/2 );
                    boxwh.width  =  static_cast<double>(dest_ptr[shape*i+2]);
                    boxwh.height =  static_cast<double>(dest_ptr[shape*i+3]);       
                        xywh_boxes.push_back(boxwh);
                        scores.push_back(dest_ptr[shape*i+4]); 
                        indices_body.push_back(i);
                    std::vector<float> key_point(key_point_num*3);
                    for(int j=0;j<key_point_num*3;j++)
                    {
                        key_point[j]=dest_ptr[shape*i+5+j];
                    } 
                        key_points.push_back( key_point );                  
                }
            }

            std::vector<int> indices_body_copy( indices_body.size() );
            for(int i=0;i<indices_body_copy.size();i++)
            {
                indices_body_copy[i]=i;
            }

            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);

            for(int i=0; i< indices_body_copy.size();i++)
            {
                int index = indices_body_copy[i];

                std::vector<float> temp_output( 5 + key_point_num*3);
                temp_output[0]= (xywh_boxes[index].x - pad_w640)*scale;
                temp_output[1]= (xywh_boxes[index].y - pad_h640)*scale;
                temp_output[2]= (xywh_boxes[index].width + xywh_boxes[index].x - pad_w640)*scale;
                temp_output[3]= (xywh_boxes[index].height + xywh_boxes[index].y - pad_h640)*scale;
                temp_output[4]= scores[index];

                for(int j=0;j<key_point_num;j++)
                {
                    temp_output[5+3*j+0] = (key_points[index][3*j]-pad_w640)*scale;
                    temp_output[5+3*j+1] = (key_points[index][3*j+1]-pad_h640)*scale;
                    temp_output[5+3*j+2] = key_points[index][3*j+2];      
                }
                output.emplace_back(temp_output);
            }
            return output;
        }
        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src,int& pad_h640,int& pad_w640,  cv::Size input_shape = cv::Size(640, 640) )
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);

            cv::Mat cut_image;

            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));

            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h640 = int((input_shape.height - cut_image.rows) /2 ) ; 

                pad_w640 = int((input_shape.width - cut_image.cols) /2 ) ; 

                cv::copyMakeBorder(cut_image, mask_image, pad_h640, input_shape.height-cut_image.rows-pad_h640, pad_w640, input_shape.width-cut_image.cols-pad_w640, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }

            else 
            {
                src.copyTo(mask_image);     
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return {mask_image,scale};

        }

      
        std::vector<int> nms_process(std::vector<std::vector<float>>& nms_input, float threshold=0.0,float iou_thres=0.9 )
        {
                        std::vector<cv::Rect2d> xywh_boxes(nms_input.size());;
            std::vector<float> scores(nms_input.size());
            std::vector<int> indices_body(nms_input.size());;//候选框顺序

            for (size_t i = 0; i < nms_input.size(); i++)
            {
                cv::Rect2d boxwh;
                boxwh.x      =  nms_input[i][0];
                boxwh.y      =  nms_input[i][1];
                boxwh.width  =  nms_input[i][2];
                boxwh.height =  nms_input[i][3];   
                xywh_boxes[i]=boxwh;
                scores[i] = nms_input[i][4];   
                indices_body[i]=i;
            }
            std::vector<int> indices_body_copy( indices_body.size() );
            for(int i=0;i<indices_body_copy.size();i++)           
                indices_body_copy[i]=i;
            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);
          
            return indices_body_copy;
        }


    private:
        std::string model_directory_;
        int device_; 
        int model_type_=1;
        int keypoint_num=17;

        std::vector<std::string>  out_names640;
        std::vector<std::string>  out_names1280;

        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;
        std::vector<float> posture_add_weight_1280single;
        std::vector<float> posture_mul_weight_1280single;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
       std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect640_;    
       std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect1280_single_branch_;    
#else
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect640_;  
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect1280_single_branch_;  
#endif

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

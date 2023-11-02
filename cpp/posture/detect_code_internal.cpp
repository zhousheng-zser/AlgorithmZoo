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
#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_rotate.hpp"
#include "Primitives/tensor_conversions.hpp"

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
                net_detect_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture17.rknn", device);     
           }
           else
           {
                net_detect_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("posture", false),
                    std::string(model_directory) + "/" +"posture12.rknn", device);     
           }
           
#else
            if(model_type_==1)
            {
                net_detect_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                std::string(model_directory) + "/" +"posture17.racy", device);      
            }
            else
            {
                net_detect_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("posture", false),
                std::string(model_directory) + "/" +"posture12.racy", device);   
            }
    #endif
            init_data();
        }

std::string version()
        {
			const std::string algo_module_version = "3.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string nn_frame_version = net_detect_->version();
#else
			std::string nn_frame_version = net_detect_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }


        exposing::param_vector<posture::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            // std::cout<<"in posture\n";
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
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
            {
                throw exposing::abi_invalid_argument("incorrect roi in posture");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;

            std::tie(blob, ratio) = preprocess_detection( cropped_image,pad_h,pad_w, new_shape ) ;

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
            auto network_result = net_detect_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
           
            for (size_t i=0;i< out_names.size(); i++)
            {
                forwards.push_back(network_result[out_names[i]]);
            }

            auto real_output = Posture_Concat(forwards, keypoint_num);

            auto nms_result = post_process(real_output,pad_h,pad_w, 1.f/ratio,keypoint_num, con_thres,iou_thres);

            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result;

            for (auto& body : nms_result)
            {
                box_info_internal temp_result;
                temp_result.x1=body[0]+ roi_x;
                temp_result.y1=body[1]+ roi_y;
                temp_result.x2=body[2]+ roi_x;
                temp_result.y2=body[3]+ roi_y;
                temp_result.score=body[4];
                temp_result.key_points = exposing::make_param_vector<float>();
                for(int j=0;j<keypoint_num;j++)
                {
                    temp_result.key_points.push_back(body[3*j+5] + roi_x);
                    temp_result.key_points.push_back(body[3*j+5+1] + roi_y);
                    temp_result.key_points.push_back(body[3*j+5+2]);
                }
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
            {
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));
            }

            return fin_result;
        }

    
    private:      
        
        void init_data()
        {
            std::vector<std::string>  out_name_12keypoint={"413","398","output0", "368" };
            std::vector<std::string>  out_name_17keypoint={  "417","402","output0","372"};

            if(model_type_==0)
            {
                out_names=out_name_12keypoint;
                keypoint_num=12;
            }
            else
            {
                 out_names=out_name_17keypoint;
                 keypoint_num=17;
            }

            posture_add_weight.resize(8400*2);
            posture_mul_weight.resize(8400);
            for(int i=0;i<8400;i++)
            {
                if( i<6400)
                {
                    posture_add_weight[i]=i%80;
                }
                else if(i<8000)
                {
                    posture_add_weight[i]=(i -6400)%40;
                }
                else
                {
                    posture_add_weight[i]=(i -8000)%20;
                }

                if( i<6400)
                {
                    posture_add_weight[i+8400]=i/80;
                }
                else if(i<8000)
                {
                    posture_add_weight[i+8400]=(i -6400)/40;
                }
                else
                {
                    posture_add_weight[i+8400]=(i -8000)/20;
                }

                if(i<6400)
                {
                    posture_mul_weight[i]=8.f;
                } 
                else if(i<8000)
                {
                    posture_mul_weight[i]=16.f;
                }
                else
                {
                    posture_mul_weight[i]=32.f;
                }
            }
        }


        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, 
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
                temp_output[0]= (xywh_boxes[index].x - pad_w)*scale;
                temp_output[1]= (xywh_boxes[index].y - pad_h)*scale;
                temp_output[2]= (xywh_boxes[index].width + xywh_boxes[index].x - pad_w)*scale;
                temp_output[3]= (xywh_boxes[index].height + xywh_boxes[index].y - pad_h)*scale;
                temp_output[4]= scores[index];

                for(int j=0;j<key_point_num;j++)
                {
                    temp_output[5+3*j+0] = (key_points[index][3*j]-pad_w)*scale;
                    temp_output[5+3*j+1] = (key_points[index][3*j+1]-pad_h)*scale;
                    temp_output[5+3*j+2] = key_points[index][3*j+2];      
                }
                output.emplace_back(temp_output);
            }
            return output;
        }
        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src,int& pad_h,int& pad_w,  cv::Size input_shape = cv::Size(640, 640) )
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);

            cv::Mat cut_image;

            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));

            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) /2 ) ; 

                pad_w = int((input_shape.width - cut_image.cols) /2 ) ; 

                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height-cut_image.rows-pad_h, pad_w, input_shape.width-cut_image.cols-pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }

            else 
            {
                src.copyTo(mask_image);     
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return {mask_image,scale};

        }

        std::shared_ptr<memory::tensor<float>> Posture_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, int key_point_num)
        {
            const int candidate_num=8400;
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 5+key_point_num*3, candidate_num}, -1, memory::NCHW));

            //20 40 80 keypoint
            const float *data80=outs[2]->cpu_data();
            const float *data40=outs[1]->cpu_data();
            const float *data20=outs[0]->cpu_data();
            const float *posture_ptr = outs[3]->cpu_data();
           
            //concat the 80*40 40*40 20*20 to  cat<vector>
             std::vector<float> cat(65*candidate_num);//1*65*8400 = 64*8400 + 1*8400
            for(int i=0;i<65;i++)
            {   
                int j=0;
                for(; j<6400; j++)
                {
                    cat[ i*candidate_num + j] = data80[i*6400 + j];
                }
                for(; j<8000; j++)
                {
                    cat[ i*candidate_num + j] = data40[i*1600 + j-6400];
                }              
                for(; j<8400; j++)
                {
                    cat[ i*candidate_num + j] = data20[i*400 + j-8000 ];
                }
            }

            //process the candidate xywh begin  
            //tranpose and softmax
            std::vector<float> reshape_box(candidate_num*64);
            tranpose(cat.data(),reshape_box.data(),64,8400 );

            int index = 0;
            for(int i=0; i<candidate_num; i++)
            {
                for(int j=0; j<4; j++)
                {
                    Softmax(reshape_box.data()+ 16*index ,16 ) ;
                    index++ ;
                }
            }

            //reshape and tranpose  64*8400 ->8400*64
            std::vector<float> reshape_box2(16*4*candidate_num);
            for(int i=0; i<candidate_num; i++)
            {
                for(int j=0; j<4; j++)
                {
                    for(int k=0; k<16; k++)
                    {
                        reshape_box2[k*4*candidate_num +j*candidate_num +i ] = reshape_box[i*16*4 + j*16+k ];
                    }
                }
            }

            //16个通道 1*1卷积
            std::vector<float> conv(4*candidate_num);
            for(int i=0;i<4*candidate_num;i++)
            {
                conv[i]=0.f;
            }
            for(int i=0;i<16;i++)
            {
                for(int j=0;j<4*candidate_num;j++)
                {
                    int location = 4*candidate_num;
                    conv[j] = conv[j] +reshape_box2[i*location+j ]*i  ; 
                }
            }

            std::vector<float>  concat(candidate_num*24);
            for(int i=0;i<candidate_num*2;i++)
            {
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f +posture_add_weight[i] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );                 // add_data[i]-sub_data[i]) ;  
            }
            //process the candidate xywh end  
        
            //process the candidate keypoint begin  
            std::vector<float> PostureXy_Conf(key_point_num*3*candidate_num);
            for(int m=0;m<key_point_num;m++)
            {
                for (size_t j = 0; j <candidate_num ; j++)
                {
                    int index = m*candidate_num*3  +j;
                    PostureXy_Conf[index]                   = ((posture_ptr[index]*2 + posture_add_weight[j])*posture_mul_weight[j]); 
                    PostureXy_Conf[index+candidate_num]     = ((posture_ptr[index+candidate_num]*2 + posture_add_weight[candidate_num+j])*posture_mul_weight[j]); 
                    PostureXy_Conf[index +candidate_num*2 ] = sigmoid_x(posture_ptr[index +candidate_num*2]) ;//最右侧sigmoid
                }            
            }   
            //process the candidate keypoint end

            //concat the output
            float * output=output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                concat[candidate_num*0 +i] = concat[candidate_num*0 +i]*posture_mul_weight[i];
                concat[candidate_num*1 +i] = concat[candidate_num*1 +i]*posture_mul_weight[i];
                concat[candidate_num*2 +i] = concat[candidate_num*2 +i]*posture_mul_weight[i];
                concat[candidate_num*3 +i] = concat[candidate_num*3 +i]*posture_mul_weight[i];

                output[candidate_num*0 +i]= concat[candidate_num*0 +i];
                output[candidate_num*1 +i]= concat[candidate_num*1 +i];
                output[candidate_num*2 +i]= concat[candidate_num*2 +i];
                output[candidate_num*3 +i]= concat[candidate_num*3 +i];

                output[candidate_num*4 +i]=  sigmoid_x(cat[candidate_num*64 +i]);
            }

            std::memcpy(output+5*candidate_num, PostureXy_Conf.data(), key_point_num*3*candidate_num*sizeof(float));
            return  output0;
        }


    private:
        std::string model_directory_;
        int device_; 
        int model_type_=1;
        int keypoint_num=17;

        std::vector<std::string>  out_names;

        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
       std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_;    
#else
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_;  
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

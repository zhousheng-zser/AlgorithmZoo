#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <RKNN2Wrapper/rknn2_wrapper.hpp>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::tumble
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("tumble", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :detect_instance_(phai,  model_directory + std::string("/tumble_sim.rknn"), device), model_directory_(model_directory)
        {
            init_data();
        }       

        exposing::param_vector<tumble::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            auto result = exposing::make_param_vector<box_info>();

            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in tumble");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();
            
            auto detect_result = run_detect(cropped_image, param_map);

            for (auto& i : detect_result)
            {
                i.x1+=roi_x;
                i.x2+=roi_x;
                i.y1+=roi_y;
                i.y2+=roi_y;
                
                i.x1= i.x1>0?i.x1:0;
                i.x1= i.x1<width?i.x1:width;

                i.x2= i.x2>0?i.x2:0;
                i.x2= i.x2<width?i.x2:width;

                i.y1= i.y1>0?i.y1:0;
                i.y1= i.y1<height?i.y1:height;

                i.y2= i.y2>0?i.y2:0;
                i.y2= i.y2<height?i.y2:height;

                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }    

            return result;
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = detect_instance_.version();
#else
			std::string nn_frame_version = detect_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:


        /**
		 * @fun sigmoid_x
		 * @param 
		 * @return sigmoid(x)
		 */
		inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}


        void init_data()
        {
        
            posture_add_weight_1280.resize(33600*2);
            posture_mul_weight_1280.resize(33600);
            for (size_t i = 0; i < 33600; i++)
            {
                if(i<25600)
                {
                    posture_add_weight_1280[i] = i%160;
                    posture_add_weight_1280[i+33600] = i/160 ;
                    posture_mul_weight_1280[i] =8.f;
                }
                else if( i<32000)
                {
                    posture_add_weight_1280[i] = (i -25600)% 80;
                    posture_add_weight_1280[i+33600] = (i-25600)/80;
                    posture_mul_weight_1280[i] = 16.f;
                }
                else
                {
                    posture_add_weight_1280[i] = (i -32000)% 40;
                    posture_add_weight_1280[i+33600] = (i-32000)/40;
                    posture_mul_weight_1280[i] = 32.f;
                }
            }
           
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

            // cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2HSV);

            // cv::Scalar lower_black(0, 0, 0); // 下限颜色 (B, G, R)
            // cv::Scalar upper_black(180, 255, 60); // 上限颜色 (B, G, R)
            
            // cv::Mat black_mask;

            // cv::inRange(mask_image, lower_black, upper_black, black_mask);
            
            // for (int i=0; i<mask_image.rows; i++)
            // {   
            //     for (int j=0;j<mask_image.cols;j++)
            //     {
            //         if(black_mask.at<uchar>(i,j)>0 )
            //         {
            //             mask_image.at<cv::Vec3b>(i,j)[0] = 0;
            //             mask_image.at<cv::Vec3b>(i,j)[1] = 0;
            //             mask_image.at<cv::Vec3b>(i,j)[2] = 62;//65
            //         }
            //     }
            // }        
            // cv::cvtColor(mask_image,mask_image, cv::COLOR_HSV2BGR);

            return {mask_image,scale};
        }
        /**
        * @fun sigmoid
        */ 
        static inline float sigmoid(float x) {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        /**
         * @fun Softmax
         * @param data, num
         * @return softmax(data) between stride 
         * @detail
         */
        void  Softmax(float *data, int num )
        {
            float sum = 0.f;
            float temp[16] = {0};

            // find max value in data
            float max = data[0];
            for(int i = 1; i < num; i++)
            {
                if(data[i] > max)
                {
                    max = data[i];
                }
            }

            for(int i = 0; i < num; i++)
            {
                temp[i] = exp(data[i] - max);

                sum += temp[i];
            }
            for(int i = 0; i < num; i++)
            {
                data[i] = temp[i] / sum;
            }
        }

        inline float de_sigmoid(float x)
        {
            if(x>=1 ||x<0)
                return NAN;
            return static_cast<float> (log( x/(1-x)));
        }


        void tranpose(const float* sou,
                            float* dest,int sourows,int soucols)
        {
            for(int i=0;i< sourows;i++)
            {
                for(int j=0;j< soucols;j++)
                {
                    dest[j*sourows+i]=sou[ i * soucols + j];    
                }
            }
        }


        std::shared_ptr<memory::tensor<float>> Yovo8se_Concat_Test2(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf,int& candicate_num)
        {
            conf =de_sigmoid(conf);
            int candidate_num=33600;
            int class_num = 65;        
            int stride_8=160;
            int stride_16=80;
            int stride_32=40;
            //20 40 80 
            const float *data160 = outs[2]->cpu_data();
            const float *data80 = outs[1]->cpu_data();
            const float *data40 = outs[0]->cpu_data();

            std::vector<int> match_index;

            const float* data160_conf = data160+stride_8*stride_8*65;
            for (size_t i = 0; i < stride_8*stride_8; i++)
                if(data160_conf[i] >conf  )             
                    match_index.push_back(i);

            const float* data80_conf = data80+stride_16*stride_16*65;
            for (size_t i = 0; i < stride_16*stride_16; i++)
                if( data80_conf[i] >conf )
                    match_index.push_back(i+25600);

            const float* data40_conf = data40+stride_32*stride_32*65;
            for (size_t i = 0; i < stride_32*stride_32; i++)
                if( data40_conf[i] >conf  )
                    match_index.push_back(i+32000);

            //concat the 80*40 40*40 20*20 
            std::vector<float> cat(66*candidate_num);//1*65*8400 = 64*8400 + 1*8400
            for(int i=0;i<66;i++)
            {
                int j=0;
                for(; j<stride_8*stride_8; j++)
                    cat[ i*candidate_num + j] = data160[i*stride_8*stride_8 + j];
                for(; j<stride_16*stride_16+stride_8*stride_8; j++)
                    cat[ i*candidate_num + j] = data80[i*stride_16*stride_16 + j-25600];     
                for(; j<stride_32*stride_32+stride_16*stride_16+stride_8*stride_8; j++)
                    cat[ i*candidate_num + j] = data40[i*stride_32*stride_32 + j-32000 ];
            }

            //process the candidate xywh begin  
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
            {
                for(int j=0; j<4; j++)
                {
                    Softmax(reshape_boxtmp.data()+ 16*index ,16 ) ;
                    index++ ;
                }
            }

            std::vector<float> reshape_box2(16*4*candidate_num);
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    for(int k=0; k<16; k++)
                        reshape_box2[k*4*candidate_num +j*candidate_num +i ] = reshape_boxtmp[i*16*4 + j*16+k ];

            //16个通道 1*1卷积
            std::vector<float> conv(4*candidate_num,0);
            
            for(int i=0;i<16;i++)
            {
                for(int j=0;j<4*candidate_num;j++)
                {
                    int location = 4*candidate_num;
                    conv[j] = conv[j] +reshape_box2[i*location+j ]*i  ; 
                }
            }

            std::vector<float>  concat(candidate_num*4);
            for(int i=0;i<candidate_num*2;i++)
            {              
                int index = match_index[i];
                if(i>=candidate_num  )          
                    index = match_index[i - candidate_num ]+33600;
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + posture_add_weight_1280[ index] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );      // add_data[i]-sub_data[i]) ;  
            }

            //concat the output
            float * output = output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                output[candidate_num*0 +i] = concat[candidate_num*0 +i]*posture_mul_weight_1280[ match_index[i]];    
                output[candidate_num*1 +i] = concat[candidate_num*1 +i]*posture_mul_weight_1280[ match_index[i]];
                output[candidate_num*2 +i] = concat[candidate_num*2 +i]*posture_mul_weight_1280[ match_index[i]];
                output[candidate_num*3 +i] = concat[candidate_num*3 +i]*posture_mul_weight_1280[ match_index[i]];
                output[candidate_num*4 +i] =  sigmoid_x(cat[33600*65 +match_index[i]]);
            }          
            return  output0;
        }
       
        /*
        @fun post_process
        */
        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, cv::Mat & blob, int pad_h, int pad_w, float scale, int num,float threshold=0.45,float iou_thres=0.6 )
        {
            std::vector<std::vector<float>> output;

            int shape =5;
            const int candidate_num=num;
            std::shared_ptr<glasssix::memory::tensor<float>> dest 
                    (new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));

            tranpose( net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
            const float *dest_ptr = dest->cpu_data(); 

            std::vector<float>  scores;
            std::vector<int>    indices_body;//候选框顺序
            std::vector<cv::Rect2d> xywh_boxes;
            std::vector<std::vector<float>> key_points;

            for(int i=0;i<candidate_num;i++)
            {
                if(dest_ptr[shape*i+4]>threshold)
                { 
                    indices_body.push_back(i);
                    cv::Rect2d boxwh;
                    boxwh.x      =  static_cast<double>(dest_ptr[shape*i] - dest_ptr[shape*i+2] / 2 );
                    boxwh.y      =  static_cast<double>(dest_ptr[shape*i+1] - dest_ptr[shape*i+3]/2 );
                    boxwh.width  =  static_cast<double>(dest_ptr[shape*i+2]);
                    boxwh.height =  static_cast<double>(dest_ptr[shape*i+3]);       
                    { 
                        xywh_boxes.push_back(boxwh);
                        scores.push_back(dest_ptr[shape*i+4]); 
                        indices_body.push_back(i);
                    }
                }
            }

            std::vector<int> indices_body_copy( indices_body.size());
            for(int i=0;i<indices_body_copy.size();i++)
            {
                indices_body_copy[i]=i;
            }
            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);

            for(int i=0; i< indices_body_copy.size();i++)
            {
                int index = indices_body_copy[i];
                std::vector<float> temp_output(5);
                temp_output[0]= (xywh_boxes[index].x - pad_w)*scale;
                temp_output[1]= (xywh_boxes[index].y - pad_h)*scale;
                temp_output[2]= (xywh_boxes[index].width + xywh_boxes[index].x - pad_w)*scale;
                temp_output[3]= (xywh_boxes[index].height + xywh_boxes[index].y - pad_h)*scale;
                temp_output[4]= scores[index];
                output.emplace_back(temp_output);
            }           
            int k=0;
            return output;
        }


        /**
        * @fun run_detect
        */
        std::vector<tumble::box_info_internal> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.25f;
            float iou_thres  = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;     

            // std::cout<<"conf_thres:"<<conf_thres<<std::endl;
            // preprocess
            auto new_shape = cv::Size(1280,  1280);

            auto output_shape = cv::Size(image.cols, image.rows);
            
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            std::vector<std::string>  phais;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;
            auto  network_results = detect_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"355","340","output0"};
            for (size_t i=0;i< out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_results[out_names[i]]);
            }
            int num =8400;
            auto real_output = Yovo8se_Concat_Test2(forwards, conf_thres, num);//5*8400

            auto nms_result = post_process(real_output, blob,pad_h,pad_w, 1.f/ratio,num);

            std::vector<tumble::box_info_internal> detect_result;

            for(auto& people:nms_result)
            {
                tumble::box_info_internal box_info;
                box_info.x1 =std::round( people[0])>0?std::round( people[0]):0  ;
                box_info.y1 =std::round( people[1])>0?std::round( people[1]):0  ;
                box_info.x2 =std::round( people[2])<image.cols?std::round( people[2]):image.cols ;
                box_info.y2 =std::round( people[3])<image.rows?std::round( people[3]):image.rows ;
                box_info.score = people[4];
                
                box_info.category = 1;

                detect_result.push_back(box_info);
            }

           return detect_result;
        }

    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper detect_instance_;
        std::vector<float> posture_add_weight_1280;
        std::vector<float> posture_mul_weight_1280;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<tumble::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

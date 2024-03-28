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
            :detect_instance_(phai,  model_directory + std::string("/tumble_sim.rknn"), device), model_directory_(model_directory), class_instance_(phai,  model_directory + std::string("/tumble_cls.rknn"), device)
        {
            init_data_compatible(1280,1280);
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
			const std::string algo_module_version = "2.2.3";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = detect_instance_.version();
#else
			std::string nn_frame_version = detect_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:

		inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

        std::tuple<int,float> get_max_index(const float * result, int num)
        {
            float max = result[0];
            int index =0;
            for (size_t i = 1; i < num; i++)
                if( result[i]>max )
                {
                    index = i;
                    max = result[i];
                }
            return {index, max};
        }

        void init_data_compatible(int width,int height)
        {
            int size_mul_weight = width*height*21/1024; //33600
            int size_add_weight = 2*size_mul_weight;  
            int width_base = width/8;
            int height_base = height/8;
            int candicate_area = width_base*height_base; //160*160

            add_weight.resize(size_add_weight);
            mul_weight.resize(size_mul_weight);
            for (size_t i = 0; i < candicate_area*21/16; i++)
            {     
                if(i< candicate_area  ) 
                {
                    add_weight[i] = i%(width_base); 
                    add_weight[i+size_mul_weight] = i/(width_base) ; //
                    mul_weight[i] = 8.f;
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

        static inline float sigmoid(float x) {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        void  Softmax(float* data, int num )
        {             
            double L2_Sum=0.f;
            for(size_t i=0; i<num; i++) 
            {
                data[i]= ( exp(data[i] ) );
                L2_Sum +=  data[i];
            }
            for(size_t i=0; i<num; i++) 
                data[i] =  data[i] / L2_Sum ;
        }

        inline float de_sigmoid(float x)
        {
            if(x>=1 ||x<0)
                return NAN;
            return static_cast<float> (log( x/(1-x)));
        }

        void tranpose(const float* sou, float* dest, int sourows, int soucols)
        {
            for(int i=0;i< sourows;i++)
                for(int j=0;j< soucols;j++)
                    dest[j*sourows+i]=sou[ i * soucols + j];    
        }

        void box_result_move_to_disjoint_region(std::vector<std::vector<float>>&sou_data, std::vector<int>& category_mask, int bias=100000 )
        {
            for (size_t i = 0; i < sou_data.size(); i++)
                sou_data[i][0] =  sou_data[i][0]+ category_mask[i]*bias;      
        }

        std::shared_ptr<memory::tensor<float>> Yovo8se_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf,int& candidate_num, std::vector<int>& category_mask,const int* add_weight, const int* mul_weight, int width = 1280, int height =1280 )
        {
            conf = de_sigmoid(conf);
            int input = width*height;
            int category = outs[0]->channels() - 64 ;
            int box_tmp_size = 64;
            int stride_8_num = input / 64;
            int stride_16_num = input / 256;
            int stride_32_num = input / 1024;

            int totol_size = stride_8_num + stride_16_num + stride_32_num ;    
            //20 40 80 
            const float *data_stride_8 = outs[2]->cpu_data();
            const float *data_stride_16 = outs[1]->cpu_data();
            const float *data_stride_32 = outs[0]->cpu_data();

            std::vector<int> match_index;
#ifdef lxyyolo
            const float* data_stride_8_conf = data_stride_8 ;
            const float* data_stride_16_conf = data_stride_16;
            const float* data_stride_32_conf = data_stride_32;  
#else
            const float* data_stride_8_conf = data_stride_8 + stride_8_num*64;
            const float* data_stride_16_conf = data_stride_16 + stride_16_num*64;
            const float* data_stride_32_conf = data_stride_32 + stride_32_num*64;
#endif 

            std::vector<float> cat(category*totol_size); //1*65*candidate_num = 64*candidate_num + 1*candidate_num        
            for(int i=0; i<category; i++)
            {   
                std::copy(data_stride_8_conf+i*stride_8_num, data_stride_8_conf+(i+1)*stride_8_num, cat.data()+i*totol_size ); 
                std::copy(data_stride_16_conf+i*stride_16_num, data_stride_16_conf+(i+1)*stride_16_num, cat.data()+i*totol_size+stride_8_num ); 
                std::copy(data_stride_32_conf+i*stride_32_num, data_stride_32_conf+(i+1)*stride_32_num, cat.data()+i*totol_size+stride_8_num+stride_16_num ); 
            }

            for (size_t i = 0; i < cat.size(); i++ )
                if( cat[i] > conf  )    
                {
                    match_index.push_back(i%totol_size);
                    category_mask.push_back(i/ totol_size);
                }

            if(!match_index.size())
            {
                std::shared_ptr<glasssix::memory::tensor<float>> output0;
                return output0;
            }
            
            //tranpose and softmax
            std::vector<float> reshape_box(totol_size*64);
#ifdef lxyyolo
            tranpose(data_stride_8  + stride_8_num*category   ,reshape_box.data(), 64, stride_8_num );
            tranpose(data_stride_16 + stride_16_num*category  ,reshape_box.data()+stride_8_num*64 ,64,stride_16_num );
            tranpose(data_stride_32 + stride_32_num*category  ,reshape_box.data()+(stride_8_num+stride_16_num)*64 , 64,stride_32_num );
#else
            tranpose(data_stride_8,reshape_box.data(), 64, stride_8_num );
            tranpose(data_stride_16,reshape_box.data()+stride_8_num*64 ,64,stride_16_num );
            tranpose(data_stride_32,reshape_box.data()+(stride_8_num+stride_16_num)*64 , 64,stride_32_num );
#endif 
            candidate_num = match_index.size();
            std::vector<float> reshape_boxtmp(candidate_num*64);
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));
        
            for (size_t i = 0; i < match_index.size(); i++)
                std::copy(reshape_box.data()+match_index[i]*64, reshape_box.data()+match_index[i]*64+64, reshape_boxtmp.data()+i*64);

            int index = 0;
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    Softmax(reshape_boxtmp.data()+ 16*index++ ,16 ) ;//inplace softamax

            std::vector<float> reshape_box2(16*4*candidate_num);
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    for(int k=0; k<16; k++)
                        reshape_box2[k*4*candidate_num +j*candidate_num +i ] = reshape_boxtmp[i*16*4 + j*16+k ];

            //16 channels 1*1convolution
            std::vector<float> conv(4*candidate_num,0);
            for(int i=0;i<16;i++)
                for(int j=0;j<4*candidate_num;j++)
                    conv[j] = conv[j] +reshape_box2[i*4*candidate_num+j ]*i; 

            std::vector<float> concat(candidate_num*4);
            float * output = output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {              
                output[candidate_num*0 +i]= ((conv[i+candidate_num*2] - conv[i] )/2.f + add_weight[match_index[i]] + 0.5)*mul_weight[ match_index[i%candidate_num]] ;     
                output[candidate_num*1 +i]= ((conv[i+candidate_num*3] - conv[i+candidate_num] )/2.f + add_weight[match_index[i]+totol_size ] + 0.5)*mul_weight[ match_index[i%candidate_num]] ;    
                output[candidate_num*2 +i]= (conv[i+candidate_num*2]  + conv[i] ) * mul_weight[ match_index[i%candidate_num]];      
                output[candidate_num*3 +i]= (conv[i+candidate_num*3]  + conv[i+candidate_num] ) * mul_weight[ match_index[i%candidate_num]]; 
                output[candidate_num*4 +i]=  sigmoid_x(cat[totol_size*category_mask[i] + match_index[i]]);
            }  
            return  output0;
        }
       
        std::vector<std::vector<float>> XYXY2WH(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale,
                        int candicate_num, std::vector<int>& category_mask, float threshold=0.0,float iou_thres=0.8 )
        {
                std::vector<std::vector<float>> output;
                if(!candicate_num )
                    return output;
                int shape = 5;
                const int candidate_num = candicate_num;
                std::shared_ptr<glasssix::memory::tensor<float>> dest 
                        (new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));
                tranpose( net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
                const float *dest_ptr = dest->cpu_data(); 

                std::vector<cv::Rect2d> xywh_boxes;
                std::vector<std::vector<float>> key_points;
                std::vector<float> scores;

                for(int i=0;i<candidate_num;i++)
                {
                        std::vector<float> temp(5);
                        cv::Rect2d boxwh;
                        boxwh.x      =  static_cast<double>((dest_ptr[shape*i] - dest_ptr[shape*i+2] / 2) - pad_w)*scale;
                        boxwh.y      =  static_cast<double>((dest_ptr[shape*i+1] - dest_ptr[shape*i+3] / 2)- pad_h)*scale;
                        boxwh.width  =  static_cast<double>(dest_ptr[shape*i+2])*scale ;
                        boxwh.height =  static_cast<double>(dest_ptr[shape*i+3])*scale ;       

                        temp[0]=boxwh.x;
                        temp[1]=boxwh.y;
                        temp[2]=boxwh.width;
                        temp[3]=boxwh.height;
                        temp[4]=dest_ptr[shape*i+4];

                        output.push_back(temp);
                }
                return output ;
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

        std::vector<tumble::box_info_internal> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_thres  = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;     

            auto new_shape = cv::Size(1280,  1280);
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;
            auto  network_results = detect_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<int> category_mask;
            std::vector<std::string>  out_names={"355","340","output0"};
            for (size_t i=0;i< out_names.size(); i++)//对输出数据做处理
                forwards.push_back(network_results[out_names[i]]);

            int candicate_num =0;
            auto real_output = Yovo8se_Concat(forwards,conf_thres,candicate_num,category_mask, add_weight.data(), mul_weight.data(), 1280,1280);//5*8400
            auto nms_input  =  XYXY2WH(real_output, pad_h, pad_w, 1.f/ratio, candicate_num, category_mask);
            box_result_move_to_disjoint_region( nms_input, category_mask, 100000);
            auto nms_result_index = nms_process(nms_input, conf_thres, iou_thres);
            box_result_move_to_disjoint_region( nms_input, category_mask, -100000);

            std::vector<tumble::box_info_internal> detect_result;
            for (size_t i = 0; i < nms_result_index.size(); i++)
            {   
                int index = nms_result_index[i];

                yolo_result result( int(nms_input[index][0]),int(nms_input[index][1]),
                                    int(nms_input[index][0]+nms_input[index][2]),  int(nms_input[index][1]+nms_input[index][3]),category_mask[index],nms_input[index][4] );    
                
                auto candidate_box = result.safe_yolo_result(image.cols,image.rows);

                auto crop_img = image(cv::Range(candidate_box.y1 , candidate_box.y2), cv::Range(candidate_box.x1, candidate_box.x2)).clone();

                auto class_new_shape = cv::Size(256,  256);
                cv::Mat class_blob;
                float class_ratio = 0;
                int class_pad_h=0;  
                int class_pad_w=0;
                std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
              
                std::tie(class_blob, class_ratio) = preprocess_detection(crop_img, class_pad_h, class_pad_w, class_new_shape );
                
                auto class_result = class_instance_.forward(class_blob.data, { 1, class_blob.rows, class_blob.cols,class_blob.channels() }, RKNN_TENSOR_NHWC);
                for (const auto& kv : class_result) 
                {  
                    auto& value = kv.second;  
                    auto [max_index, max] = get_max_index( value->cpu_data(), value->count());
                    if(max_index==0 && max>0.6 )
                        detect_result.push_back( result.yolo_result2box() );
                }  
            }

            // std::vector<tumble::box_info_internal> detect_result;
            // for (size_t i = 0; i < nms_result_index.size(); i++)
            // {   
            //     int index = nms_result_index[i];
            //     yolo_result result( int(nms_input[index][0]),int(nms_input[index][1]),
            //                         int(nms_input[index][0]+nms_input[index][2]),  int(nms_input[index][1]+nms_input[index][3]),category_mask[index],nms_input[index][4] );
            //     if(result.category==1)
            //         detect_result.push_back( result.yolo_result2box());       
            // }

            return detect_result;
        }

    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper detect_instance_;
        glasssix::rknnwrapper::rknn_wrapper class_instance_;
        std::vector<int> add_weight;
        std::vector<int> mul_weight;
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

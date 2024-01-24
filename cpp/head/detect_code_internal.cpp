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
#include "Primitives/tensor_conversions.hpp"

#include "general.hpp"

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>

namespace glasssix::head
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_detect_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("head", false),
            std::string(model_directory) + "/" +"head_detect.rknn", device);      
#else
            net_detect_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("head", false),
            std::string(model_directory) + "/" +"head.racy", device);      
#endif  
            init_data();

        }

std::string version()
        {
			const std::string algo_module_version = "1.1.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string nn_frame_version = net_detect_->version();
#else
			std::string nn_frame_version = net_detect_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }


        exposing::param_vector<head::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
                                                        int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {

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
                throw exposing::abi_invalid_argument("incorrect roi in head");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));
            auto nms_result =  head_detect(cropped_image, con_thres, iou_thres);

            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result;

            for (auto& body : nms_result)
            {
                safe_crop_rect head_rect(body[0]+ roi_x, body[2]+ roi_x, body[1]+ roi_y, body[3]+ roi_y, width, height);
                box_info_internal temp_result;
                temp_result.x1=head_rect.x1;
                temp_result.y1=head_rect.y1;
                temp_result.x2=head_rect.x2;
                temp_result.y2=head_rect.y2;
                temp_result.score=body[4];
                //temp_result.key_points = exposing::make_param_vector<float>();
           
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
            {
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));
            }

            return fin_result;
        }

    
    private:      


        std::vector<std::vector<float>> head_detect(cv::Mat& image,float threshold, float iou_thres)
        {
            std::vector<box_info_internal> output;

			auto new_shape = cv::Size(1280, 1280);

            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;

            auto  network_results = net_detect_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"355","340","output0"};

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            for (size_t i=0;i< out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_results[out_names[i]]);
            }

            int num =8400;
            auto real_output = Yovo8se_Concat(forwards,threshold,num);//5*8400
            return post_process(real_output,pad_h,pad_w, 1.f/ratio, num,threshold,iou_thres );
      

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

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src,int& pad_h,int& pad_w,  cv::Size input_shape = cv::Size(640, 640) )
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

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, int num,float threshold=0.00,float iou_thres=0.6 )
        {
             std::vector<std::vector<float>> output;

            int shape =5;
            const int candidate_num=num;
            std::shared_ptr<glasssix::memory::tensor<float>> dest 
                    (new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));

            tranpose( net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
            const float *dest_ptr = dest->cpu_data(); 

            std::vector<float>  scores;
            std::vector<int>    indices_body;               //候选框顺序
            std::vector<cv::Rect2d> xywh_boxes;
            std::vector<std::vector<float>> key_points;

            for(int i=0;i<candidate_num;i++)
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
            return output;
        }

        std::shared_ptr<memory::tensor<float>> Yovo8se_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf,int& candicate_num)
        {
            conf = de_sigmoid(conf);
            int input = 1280;
            int box_tmp_size = 64;
            int stride_8_num = input / 8;
            int stride_16_num = input / 16;
            int stride_32_num = input / 32;

            int candidate_num = stride_8_num*stride_8_num + stride_16_num*stride_16_num + stride_32_num*stride_32_num ;
            int totol_size = stride_8_num*stride_8_num + stride_16_num*stride_16_num + stride_32_num*stride_32_num ;    
            //20 40 80 
            const float *data_stride_8 = outs[2]->cpu_data();
            const float *data_stride_16 = outs[1]->cpu_data();
            const float *data_stride_32 = outs[0]->cpu_data();

            std::vector<int> match_index;

            const float* data_stride_8_conf = data_stride_8+stride_8_num*stride_8_num*box_tmp_size;
            for (size_t i = 0; i < stride_8_num*stride_8_num; i++)
                if( data_stride_8_conf[i] >conf  )
                    match_index.push_back(i);
            const float* data_stride_16_conf = data_stride_16+stride_16_num*stride_16_num*box_tmp_size;
            for (size_t i = 0; i < stride_16_num*stride_16_num; i++)
                if( data_stride_16_conf[i]>conf )
                    match_index.push_back(i+stride_8_num*stride_8_num);
            const float* data_stride_32_conf = data_stride_32+stride_32_num*stride_32_num*box_tmp_size;
            for (size_t i = 0; i < stride_32_num*stride_32_num; i++)
                if( data_stride_32_conf[i] >conf  )    
                    match_index.push_back(i+ stride_8_num*stride_8_num + stride_16_num*stride_16_num );

            //concat the 80*40 40*40 20*20 
            std::vector<float> cat(65*candidate_num);//1*65*candidate_num = 64*candidate_num + 1*candidate_num
            for(int i=0,j=0;i<65;i++,j=0)
            {   
                for(; j<stride_8_num*stride_8_num; j++)             
                    cat[ i*candidate_num + j] = data_stride_8[i*stride_8_num*stride_8_num + j];
                for(; j<stride_16_num*stride_16_num+stride_8_num*stride_8_num; j++)              
                    cat[ i*candidate_num + j] = data_stride_16[i*stride_16_num*stride_16_num + j- stride_8_num*stride_8_num];                        
                for(; j<stride_32_num*stride_32_num+stride_16_num*stride_16_num+stride_8_num*stride_8_num; j++)
                    cat[ i*candidate_num + j] = data_stride_32[i*stride_32_num*stride_32_num + j-stride_8_num*stride_8_num  -stride_16_num*stride_16_num ];
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
            for(int i=0,index=match_index[0];i<candidate_num*2;i++)
            {              
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + posture_add_weight_1280[ i<candidate_num? match_index[i]:(match_index[i - candidate_num ]+totol_size) ] + 0.5;     
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
                output[candidate_num*4 +i] =  sigmoid_x(cat[totol_size*64 +match_index[i]]);
            }          
            return  output0;
        }
       
    private:
        std::string model_directory_;
        int device_; 
        std::vector<float> posture_add_weight_1280;
        std::vector<float> posture_mul_weight_1280;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
       std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_;    
#else
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_;  
#endif

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

    exposing::param_vector<head::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>

#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Primitives/tensor_conversions.hpp"
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <iomanip>
#include <tuple>

namespace glasssix::helmet
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("helmet", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_detect_(phai,  model_directory + std::string("/head_detect.rknn"), device),
                 net_class_(phai,  model_directory + std::string("/helmet_sim.rknn"), device)
        {

        }

        exposing::param_vector<helmet::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {

            float MIN_HEAD = param_map.count("min_size") ? param_map["min_size"] : 48.f;
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

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
                  throw exposing::abi_invalid_argument("incorrect roi in helmet");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));


            std::vector<helmet::box_info_internal> result =  helmet_detect(cropped_image, con_thres, iou_thres, MIN_HEAD);
            // std::cout<<"ok\n";
            // std::cout<<result[0].x1<<" "<<result[0].x2<<std::endl;

            auto results = exposing::make_param_vector<helmet::box_info>();

            for( auto& it:result) {
                // std::cout<<it.x1<<std::endl;
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            
            return results;
        }

        std::string version()
        {
			const std::string algo_module_version = "2.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_detect_.version();
#else
			std::string nn_frame_version = net_detect_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:
        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        struct detect_list
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
        };

         inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

        void tranpose(std::shared_ptr<memory::tensor<float>>& data,
                            std::shared_ptr<memory::tensor<float>>& dest)
        {
            const float *sour_ptr = data->cpu_data();

            float *dest_ptr = dest->mutable_cpu_data();

            int dim_2 = dest->count()/8400;

            for(int i=0;i< dim_2;i++)
            {
                for(int j=0;j< 8400;j++)
                {
                    dest_ptr[j*dim_2+i]=sour_ptr[ i * 8400 + j];    
                }
            }
        }

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, cv::Mat & blob, int pad_h, int pad_w, float scale, float threshold=0.5,float iou_thres=0.6 )
        {
            std::vector<std::vector<float>> output;

            int dim_2 = net_result->count()/8400;
            std::shared_ptr<glasssix::memory::tensor<float>> dest 
                    (new glasssix::memory::tensor<float>(8400, dim_2, -1, glasssix::memory::NCHW, nullptr));

            tranpose( net_result ,dest);

            const float *dest_ptr = dest->cpu_data(); 

            std::vector<cv::Rect2d> xywh_boxes;

            std::vector<std::vector<float>> key_points;

            std::vector<float> scores;
            std::vector<int> indices_body;//候选框顺序

            int count=0;
            for(int i=0;i<8400;i++)
            {
                if(dest_ptr[dim_2*i+4]>0.8 )
                {
                    count++;      
                    indices_body.push_back(i);
                    
                    cv::Rect2d boxwh;
                    boxwh.x      =  static_cast<double>(dest_ptr[dim_2*i] - dest_ptr[dim_2*i+2] / 2 );
                    boxwh.y      =  static_cast<double>(dest_ptr[dim_2*i+1] - dest_ptr[dim_2*i+3]/2 );
                    boxwh.width  =  static_cast<double>(dest_ptr[dim_2*i+2]);
                    boxwh.height =  static_cast<double>(dest_ptr[dim_2*i+3]);       

                    { 
                        xywh_boxes.push_back(boxwh);
                        scores.push_back(dest_ptr[dim_2*i+4]); 
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


            // cv::imwrite("../preocess.jpg",blob);

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

            // cv::imwrite("../preocesdss.jpg",blob);
            
            int k=0;
            
            return output;

        }

         std::shared_ptr<memory::tensor<float>> yolov8s_complement(std::unordered_map< std::string,std::shared_ptr<memory::tensor<float>>>& forwards)
        {
            
            long num_of_sigmoid_output = forwards["onnx::Sigmoid_380"]->count();
            int dim = num_of_sigmoid_output/8400;

            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 4+dim, 8400}, -1, memory::NCHW));

            float *concat_4_output=forwards["onnx::Mul_423"]->mutable_cpu_data();
            std::vector<float> mul_weight(8400);

            for(int i=0;i<8400;i++)
            {
                if(i<6400)
                {
                    mul_weight[i]=8.f;
                } 
                else if(i<8000)
                {
                    mul_weight[i]=16.f;
                }
                else
                {
                    mul_weight[i]=32.f;
                }
            }

            for(int i=0;i<4;i++)
            {
                for(int j=0;j<8400;j++)
                {
                    concat_4_output[i*8400+j] = concat_4_output[i*8400+j]*mul_weight[j];
                }
            }

            float *Split_output_1=forwards["onnx::Sigmoid_380"]->mutable_cpu_data();

            for(size_t i=0;i<dim*8400;i++)
            {
                Split_output_1[i] = sigmoid_x(Split_output_1[i]);
            }

            float * ptr_output = output0->mutable_cpu_data();  

            memcpy(ptr_output,        concat_4_output,8400*4* sizeof(float) ); //4个坐标维度
            memcpy(ptr_output+8400*4, Split_output_1, 8400*dim* sizeof(float) );
            return output0;

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


        std::vector<helmet::box_info_internal> helmet_detect(cv::Mat& image,float threshold, float iou_thres, float MIN_HEAD=48.f)
        {
            std::vector<box_info_internal> output;

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;
            std::map< std::string,std::shared_ptr<memory::tensor<float>>> forwards;

            unsigned char * blobdata=blob.ptr<uchar>();

            auto  network_result = net_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::shared_ptr<memory::tensor<float>> real_output = yolov8s_complement(network_result);

            auto nms_result = post_process(real_output, blob,pad_h,pad_w, 1.f/ratio, threshold,iou_thres );

            for(auto& head:nms_result)
            {
                int x1=std::round( head[0])>0?std::round( head[0]):0  ;
                int y1=std::round( head[1])>0?std::round( head[1]):0  ;
                int x2=std::round( head[2])<image.cols?std::round( head[2]):image.cols ;
                int y2=std::round( head[3])<image.rows?std::round( head[3]):image.rows ;

                if( (y2-y1)<MIN_HEAD || (x2-x1)<MIN_HEAD )
                {
                    continue;
                }

                cv::Mat crop = image(cv::Range(y1,y2), cv::Range(x1,x2));

                cv::Mat headimg;
                cv::resize(crop, headimg, cv::Size((int)(96), 
                                (int)(96)), cv::INTER_LINEAR);

                auto  network_result = net_class_.forward(headimg.data, { 1, headimg.rows, headimg.cols,headimg.channels() }, RKNN_TENSOR_NHWC);
                const float *data1=network_result["output"]->cpu_data();
                const float *data2=network_result["569"]->cpu_data();
                
                // std::cout<<"data1[0]"<<data1[0]<<std::endl;
                std::vector<float> confidenceofhelmet(2);

                float sum_confi=0.f;
                for(int i=0;i<2;i++)
                {
                    sum_confi+= exp(data2[i]);
                }

                for(int i=0;i<2;i++)
                {
                    confidenceofhelmet[i] = exp(data2[i])/sum_confi;
                }


                if(data1[1]>data1[0])
                {
                        box_info_internal  headp;
                        headp.x1=x1;
                        headp.x2=x2;
                        headp.y1=y1;
                        headp.y2=y2;      
                                                 
                        if( data2[0]<data2[1] && confidenceofhelmet[1]>0.9)
                        {
                            headp.category=2;
                            headp.score=  confidenceofhelmet[1]; 
                        }   
                        else
                        {
                            // std::cout<<"helmet"<<std::endl;
                            headp.category=0;
                            headp.score=  confidenceofhelmet[0]; 
                        }

                        output.push_back(headp);
                }
                else
                {
                    // std::cout<<"no know\n";
                }
            }
            // std::cout<<output.size()<<std::endl;
            return output;
            // std::cout<<output[0].x1<<std::endl;
            // std::cout<<output[0].y1<<std::endl;
        }


    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper net_detect_;
        glasssix::rknnwrapper::rknn_wrapper net_class_;
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

    exposing::param_vector<helmet::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "hardcode.hpp"
#include <mutex>

namespace glasssix::wander
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_detect_(phai,  model_directory + std::string("/people_detect.rknn"), device), net_feature_(phai, model_directory + std::string("/people_feature.rknn"), device), model_directory_(model_directory)
        {   
           
        }

        exposing::param_vector<wander::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in wander");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

            std::vector<wander::box_info_internal> cate_result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<wander::box_info>();

            for(auto& it:cate_result) 
            {
                // std::cout<<it.first_show_time<<std::endl;
                // std::cout<<it.last_show_time<<std::endl;
                // std::cout<<it.cosine_similarity<<std::endl;
                // std::cout<<it.confidence<<std::endl;
                // std::cout<<it.id<<std::endl;
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
        const std::string algo_module_version = "1.0.2";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
        std::string nn_frame_version = net_detect_.version();
#else
        std::string nn_frame_version = net_detect_.version();
#endif
        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    public: 
        // struct wander_info
        // {
        //     std::array<float,2048> feature;            
        //     float   feature_sqrt_xx;
        //     double first_init_time;
        //     double last_match_time;
        // };

    private:

        struct landmark
        {
           float mark[10]; 
        };
  
        struct return_info
        {
            int id;
            double first_show_time=0.f;
            double last_show_time=0.f;
            float  cosine_similarity=0.f;
            int x1;
            int x2;
            int y1;
            int y2;
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

        void  Softmax(float* data, int num )
        {        
            double L2_Sum=0.f;
            for(size_t i=0; i<num; i++) 
            {
                data[i]= ( exp(data[i] ) );
                L2_Sum +=  data[i];
            }
            for(size_t i=0; i<num; i++) 
            {
                data[i] =  data[i] / L2_Sum ;
            }       
        }
    
        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src,int& pad_h,int& pad_w,  cv::Size input_shape = cv::Size(640, 640) )
        {
            float ratio = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * ratio), (int)(src.rows * ratio)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) /2 ) ; 
                pad_w = int((input_shape.width - cut_image.cols) /2 ) ; 
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height-cut_image.rows-pad_h, pad_w, input_shape.width-cut_image.cols-pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else 
            {
                src.copyTo(mask_image);     
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return {mask_image,ratio};
        }

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, cv::Mat & blob, int pad_h, int pad_w, float scale, float threshold=0.7,float iou_thres=0.6 )
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
                if(dest_ptr[dim_2*i+4]>0.450)
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

            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, threshold, iou_thres);


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
        
        std::shared_ptr<glasssix::memory::tensor<float>> Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {
            //20 40 80
            std::vector<float> cat(65*8400);//1*65*8400 = 64*8400 + 1*8400
            const float *data80=outs[2]->cpu_data();
            const float *data40=outs[1]->cpu_data();
            const float *data20=outs[0]->cpu_data();
            // int i=0;
            int Candidate=8400;
            for(int i=0;i<65;i++)
            {   
                int j=0;
                for(; j<6400; j++)
                {
                    cat[ i*Candidate + j] = data80[i*6400 + j];
                }
                for(; j<8000; j++)
                {
                    cat[ i*Candidate + j] = data40[i*1600 + j-6400];
                }
                
                for(; j<8400; j++)
                {
                    cat[ i*Candidate + j] = data20[i*400 + j-8000 ];
                }
            }

            //boxes cat[0:64*8400]
 
            std::vector<float> reshape_box(8400*64);
            //tranpose and softmax
            for(int i=0; i<64; i++)
            {
                for(int j=0; j<8400; j++)
                {
                    reshape_box[j*64 + i] = cat[i*8400 + j ];
                }
            }
            
            int index = 0;
            for(int i=0; i<8400; i++)
            {
                for(int j=0; j<4; j++)
                {
                    Softmax(reshape_box.data()+ 16*index ,16 ) ;
                    index++ ;
                }
            }

            //reshape and tranpose  64*8400 ->8400*64
            std::vector<float> reshape_box2(16*4*8400);

            std::array<float, 64> temp;

            for(int i=0; i<8400; i++)
            {
                for(int j=0; j<4; j++)
                {
                    for(int k=0; k<16; k++)
                    {
                        reshape_box2[k*4*8400 +j*8400 +i ] = reshape_box[i*16*4 + j*16+k ];
                    }
                }
            }


            std::vector<float> conv(4*8400);

            for(int i=0;i<4*8400;i++)
            {
                conv[i]=0.f;
            }

            //16个通道 1*1卷积
            for(int i=0;i<16;i++)
            {
                for(int j=0;j<4*8400;j++)
                {
                    int location = 4*8400;
                    reshape_box2[i*location+j ] = reshape_box2[i*location+j ] * i;
                    conv[j] = conv[j] +reshape_box2[i*location+j ]; 
                }
            }

            //slice and function operator

            std::vector<float> sub_add(8400*2);

            for(int i=0; i<6400; i++)
            {
                sub_add[i]=i%80-0.5f+1.f;
            }
            for(int i=0; i<1600; i++)
            {
                sub_add[6400+i]=i%40-0.5f+1.f;
            }
            for(int i=0; i<400; i++)
            {
                sub_add[8000+i] = i%20-0.5f+1.f;
            }

             for(int i=0; i<6400; i++)
            {
                sub_add[8400+i]=i/80-0.5f+1.f;
            }
            for(int i=0; i<1600; i++)
            {
                sub_add[8400+6400+i]=i/40-0.5f+1.f;
            }
            for(int i=0; i<400; i++)
            {
                sub_add[8400+8000+i] = i/20-0.5f+1.f;
            }

            //2次sub and add   此处应该是xyxy2xywh
            std::vector<float> sub_data(8400*2);
            std::vector<float> add_data(8400*2);
            for(int i=0;i<8400*2;i++)
            {
                sub_data[i] = sub_add[i]-conv[i];
                add_data[i] = conv[i+8400*2]+sub_add[i];
            }
            
            std::vector<float> add2_data(8400*2);
            std::vector<float> sub2_data(8400*2);

            for(int i=0;i<8400*2;i++)
            {
                add2_data[i]=sub_data[i]+add_data[i];
                sub2_data[i]=add_data[i]-sub_data[i];
            }

            //div concat
            std::vector<float>  concat(8400*24);
            for(int i=0;i<8400*2;i++)
            {
                concat[i]        = add2_data[i]/2.f;     
                concat[i+8400*2] = sub2_data[i] ;   
            }
            
            std::vector<float> MUL(8400);

            for(int i=0; i<6400; i++)
            {
                    MUL[i]=8;
                if(i<1600)
                {
                    MUL[i+6400]=16;   
                }
                if(i<400)
                {
                    MUL[i+8000]=32;  
                }
            }

            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 5, 8400}, -1, memory::NCHW));
            // std::vector<float> output(5*8400);
            float * output=output0->mutable_cpu_data();
            for(int i=0;i<8400;i++)
            {
                concat[8400*0 +i] = concat[8400*0 +i]*MUL[i];
                concat[8400*1 +i] = concat[8400*1 +i]*MUL[i];
                concat[8400*2 +i] = concat[8400*2 +i]*MUL[i];
                concat[8400*3 +i] = concat[8400*3 +i]*MUL[i];

                output[8400*0 +i]= concat[8400*0 +i];
                output[8400*1 +i]= concat[8400*1 +i];
                output[8400*2 +i]= concat[8400*2 +i];
                output[8400*3 +i]= concat[8400*3 +i];
                output[8400*4 +i]=  sigmoid_x(cat[8400*64 +i]);
            }
                      
            return  output0;

        }

        static float cosine_similiar(const float *data1,const float* data2, float sqrt_xx,float sqrt_yy,int num=2048)
        {
            float xy=0.f;
            float xx=0.f;
            float yy=0.f;
            for(int i=0;i<num;i++)
            {
                xy += data1[i]*data2[i];
            }
            if(sqrt_xx * sqrt_yy<1e-7)
            {
                return -1;
            }
            return xy/(sqrt_xx*sqrt_yy); 
        }

        int get_id(std::map<int, wander_info> & feature_table,int num)//get a new allocate id
        {
            int id=num-1;
            bool full=true;
            std::vector<int> mask(num);
            for (size_t i=0;i< num; i++)//对输出数据做处理
            {
                mask[i]=1;
            }
            
            std::map<int, wander_info> ::iterator it;
            for(it=feature_table.begin(); it != feature_table.end();  it++ )
            {   
               mask[it->first]=0;
            }

            for (size_t i=0; i< num; i++)//对输出数据做处理
            {
                if(mask[i]==1)
                {
                    full=false;
                    id = i;
                    return id;
                }
            }
            // if(id==(num-1))
            // {
            //     std::cout<<"need delete\n";
            // }
            return id;
        }

        //  void delete_feature_library_one(int devices, std::map<int,std::map<int, wander_info>> & feature_tables )
        // {
        //     if(feature_tables.count(devices))
        //     {
        //         feature_tables.erase(devices);
        //     }
        // }

        //  void delete_feature_library_all(std::map<int,std::map<int, wander_info>> & feature_tables )
        // {
        //     feature_tables.clear();
        // }

        return_info feature_match(float *data,float sqrt_xx, double current_time, int devices, int table_size=200, float threshold=0.92)
        { 
            // wander_array.push_back(1);
            return_info person_result;

            //ergodic map
            //check devices if insered:get() else make a map         
            //  std::cout<<"devices id: "<<devices<<" working start!\n";
            std::map<int, wander_info> feature_table;

            if(feature_tables.count(devices))
            {   
                feature_table = feature_tables[devices];
                // std::cout<<"match devices: "<<devices<<" "<<feature_table.size()<<" "<<feature_tables[devices].size()<<std::endl;
            } 

            std::map<int, wander_info> ::iterator it;
            for(it=feature_table.begin(); it != feature_table.end();  it++ )
            {   
                float *data2 = feature_table[it->first].feature.data();
                auto similiar = cosine_similiar(data, data2, sqrt_xx,feature_table[it->first].feature[2048] );
                if(similiar>threshold)
                {
                    feature_table[it->first].last_match_time = current_time;
                    person_result.id = it->first;
                    person_result.first_show_time = feature_table[it->first].first_init_time;
                    person_result.last_show_time  = current_time;
                    person_result.cosine_similarity = similiar;
                    // std::cout<<"devices id: "<<devices<<" working end!\n";
                    return person_result;
                }
            }

            {
                auto id = get_id(feature_table, table_size);
                // std::cout<<"new id: "<<id<<std::endl;
                std::array<float,2048> feature;
                std::memcpy(feature.data(), data, 2048*sizeof(float));
                wander_info w_i;
                w_i.feature = feature;
                w_i.feature_sqrt_xx = sqrt_xx;
                w_i.first_init_time = current_time;
                feature_table[id] = w_i;     
                person_result.id =id;
                //  std::cout<<" person_result.id id: "<< person_result.id<<std::endl;
                person_result.first_show_time = current_time;
                person_result.last_show_time  = 0.f;
                person_result.cosine_similarity = 0.f;
                feature_tables[devices]=feature_table;
                //  std::cout<<"devices id: "<<devices<<" working end!\n";
            }
             return person_result;
        }

        std::vector<box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map)
        {
                // fun();
            double device_id        = param_map.count("device_id") ? param_map["device_id"] : 0;
            double feature_table_size = param_map.count("feature_table_size") ? param_map["feature_table_size"] : 10000.f;      
            double current_time        = param_map.count("current_time") ? param_map["current_time"] : 0.f;

            float feature_match_threshold  = param_map.count("feature_match_threshold") ? param_map["feature_match_threshold"] : 0.92f;
            float conf_threshold   = param_map.count("person_conf") ? param_map["person_conf"] : 0.7f;
            
            float iou_threshold =  0.45f;   

            auto new_shape = cv::Size(640,  640);
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;

                        // cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
            auto  network_results = net_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
         
            std::vector<std::string>  out_names={"355","340","output0"};
            
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
        
            for (size_t i=0;i< out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_results[out_names[i]]);
            }
   
            auto real_output = Concat(forwards, conf_threshold);//5*8400

            auto nms_result = post_process(real_output, blob,pad_h,pad_w, 1.f/ratio,conf_threshold);


            std::vector<box_info_internal> l_c; 
            for(auto& head:nms_result)
            {
                int x1=std::round( head[0])>0?std::round( head[0]):0  ;
                int y1=std::round( head[1])>0?std::round( head[1]):0  ;
                int x2=std::round( head[2])<image.cols ? std::round( head[2]):image.cols ;
                int y2=std::round( head[3])<image.rows ? std::round( head[3]):image.rows ;

                // std::cout<<x1<<" "<<x2<<" "<<y1<<" "<<y2<<" "<<head[4] <<std::endl;


                if( (y2-y1)<0 ||(x2-x1)<0 )
                {
                    continue;
                }

                cv::Mat crop = image(cv::Range(y1,y2), cv::Range(x1,x2));
                cv::Mat headimg;
                cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                cv::resize(crop, headimg, cv::Size((int)(128), (int)(256)), cv::INTER_CUBIC);
                auto  network_result = net_feature_.forward(headimg.data, { 1, headimg.rows, headimg.cols,headimg.channels() }, RKNN_TENSOR_NHWC);

                float *data1=network_result["865"]->mutable_cpu_data();

                float xx = 0.f;
                for(int i=0; i<2048; i++)
                {
                    xx += data1[i] * data1[i] ;
                }
                auto sqrt_xx=sqrt(xx);
                std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
                // std::cout<<"device: "<<std::round(device_id)<<"1\n";
                auto person_info = feature_match(data1, sqrt_xx,current_time, std::round(device_id), feature_table_size, feature_match_threshold );
                // std::cout<<"device: "<<std::round(device_id)<<"2\n";
    
                box_info_internal result;
                result.x1=x1>0?x1:0 ;
                result.y1=y1>0?y1:0 ;
                result.x2=x2<image.cols ?x2:image.cols ;
                result.y2=y2<image.rows ?y2:image.rows ;
                result.confidence = head[4] ;
                result.id = person_info.id;
                result.first_show_time = person_info.first_show_time;
                result.last_show_time = person_info.last_show_time;
                result.cosine_similarity= person_info.cosine_similarity;
                l_c.emplace_back(result);
            }
            return l_c;
        }


    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_detect_;
        rknnwrapper::rknn_wrapper net_feature_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_detect_;
        std::unique_ptr<excalibur::pipeline<float>> net_feature_;
#endif
        std::string model_directory_;
        
        int device_ ;

    public:
        static std::mutex Feature_Table_Mutex;
        static std::map<int, std::map<int, wander_info>>  feature_tables;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {

    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<wander::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

    std::map<int, std::map<int, wander_info>> detect_code_internal::impl::feature_tables;
    std::mutex detect_code_internal::impl::Feature_Table_Mutex;
}

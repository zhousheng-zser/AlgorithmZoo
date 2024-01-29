#ifndef __GENERAL_HPP__
#define __GENERAL_HPP__

#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"  
    
using namespace glasssix;

        
        // const int sleep_count_threshold=10;

        struct Sleep_trace
        {
            int m_left;
            int m_top;
            int m_width;
            int m_height;
            bool sleeping;
            int count;
            float conf;
            int sleep_count_threshold=10;

            Sleep_trace(float x1, float y1, float w, float h, float confidence,int frame_count_thres) 
            {
                m_left = x1;
                m_top = y1;
                m_width = w;
                m_height = h;
                conf = confidence;
                count = 1 ;
                sleep_count_threshold = frame_count_thres;
                sleeping = false;
            }

            Sleep_trace(int x1, int y1, int x2, int y2, bool sleep_status= false) 
            {
                m_left = x1;
                m_top = y1;
                m_width = x2 - x1;
                m_height = y2 - y1;
                count = 1 ;
                sleeping = sleep_status;
            }

            void refresh_location(Sleep_trace& info, bool sleep_status) 
            {
                m_left = info.m_left;
                m_top = info.m_top;
                m_width = info. m_width;
                m_height = info.m_height;
                conf = info.conf;
                if( count<sleep_count_threshold)
                {  
                    count++;
                    sleeping = (count==sleep_count_threshold);
                }                
            }

            bool get_sleep_status()
            {
                return count==sleep_count_threshold;
            }
        };

        float IOU_compute(const Sleep_trace& b1, const Sleep_trace& b2)
        {
            float w = std::max(b1.m_left+b1.m_width,b2.m_left+b2.m_width  ) - std::min(b1.m_left,b2.m_left);
            float h = std::max(b1.m_top+b1.m_height,b2.m_top+b2.m_height  ) - std::min(b1.m_top,b2.m_top);
            float ww = b1.m_width +b2.m_width;
            float hh = b1.m_height +b2.m_height;
            if( ww<w || hh<h)
                return 0; 
            else
                return (ww-w)*(hh-h)/static_cast<float>(b1.m_width*b1.m_height);
        }

        //传入的仅为判断为睡觉的人
        std::vector<int> sleep_trace(std::map<int, std::vector<Sleep_trace>>& sleeplibrarys, int device_id, std::vector<Sleep_trace>& current_sleep_infos)
        {
            std::vector<Sleep_trace> old_sleeplibrary;
            std::vector<Sleep_trace> new_sleeplibrary;//如果表未建立
            std::vector<int> sleep_status;

            if( sleeplibrarys.count(device_id))
            {
                old_sleeplibrary = sleeplibrarys[device_id]; 
            }
            
            for(auto& current_sleep_info : current_sleep_infos)
            {
                bool trace_success = false;
                for(auto& old_person_info : old_sleeplibrary)
                {
                    float iou = IOU_compute(old_person_info, current_sleep_info);
                    if(iou>0.8)                         //匹配成功
                    {
                        old_person_info.refresh_location(current_sleep_info,true);
                        new_sleeplibrary.push_back(old_person_info);
                        sleep_status.push_back(old_person_info.get_sleep_status() );
                        trace_success = true;
                        break;
                    }
                }
                if(!trace_success)
                {
                    new_sleeplibrary.push_back(current_sleep_info);
                    sleep_status.push_back(0 );
                }
            }
            sleeplibrarys[device_id] = new_sleeplibrary;
            return sleep_status;

        }

        inline float de_sigmoid(float x)
        {
            if(x>=1 ||x<0)
                return NAN;
            return static_cast<float> (log( x/(1-x)));
        }

         static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        void tranpose(const float* sou,
                                float* dest,int sourows,int soucols)
        {
            for(int i=0;i< sourows;i++)
                for(int j=0;j< soucols;j++)
                    dest[j*sourows+i]=sou[ i * soucols + j];                   
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

        std::shared_ptr<memory::tensor<float>> Posture_Concat640(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, int key_point_num,float conf,int& candicate_num,std::vector<float>& posture_add_weight,
                            std::vector<float>& posture_mul_weight, std::vector<int>& category_mask)
        {
            conf = de_sigmoid(conf);
            int input = 640;   // input_size: 1280*1280, only support single class
            int box_tmp_size = 64;
            int stride_8_num = input / 8;
            int stride_16_num = input / 16;
            int stride_32_num = input / 32;
            int candidate_num= stride_8_num*stride_8_num + stride_16_num*stride_16_num + stride_32_num*stride_32_num ;
            int totol_size = stride_8_num*stride_8_num + stride_16_num*stride_16_num + stride_32_num*stride_32_num;

            //20 40 80 keypoint
            const float *data_stride_8 = outs[2]->cpu_data();
            const float *data_stride_16 = outs[1]->cpu_data();
            const float *data_stride_32 = outs[0]->cpu_data();
            const float *posture_ptr = outs[3]->cpu_data();

            std::vector<int> match_index;
            const float* data_stride_8_conf = data_stride_8 + stride_8_num*stride_8_num*box_tmp_size;
            
            for (size_t j = 0; j < 2; j++)
            {
                for (size_t i = 0; i < stride_8_num*stride_8_num; i++)
                {
                    if( data_stride_8_conf[j*stride_8_num*stride_8_num + i] > conf  )
                    {
                        match_index.push_back(i);
                        category_mask.push_back(j);
                    }
                }
            }
            
            const float* data_stride_16_conf = data_stride_16 + stride_16_num*stride_16_num*box_tmp_size;
            for (size_t j = 0; j < 2; j++)
            {
                for (size_t i = 0; i < stride_16_num*stride_16_num; i++)
                {
                    if( data_stride_16_conf[j*stride_16_num*stride_16_num + i] > conf  )
                    {
                        match_index.push_back(i+stride_8_num*stride_8_num);
                        category_mask.push_back(j);
                    }
                }
            }

            const float* data_stride_32_conf = data_stride_32 + stride_32_num*stride_32_num*box_tmp_size;
            for (size_t j = 0; j < 2; j++)
            {
                for (size_t i = 0; i < stride_32_num*stride_32_num; i++)
                {
                    if( data_stride_32_conf[j*stride_32_num*stride_32_num + i] > conf  )
                    {
                        match_index.push_back(i+stride_8_num*stride_8_num + stride_16_num*stride_16_num);
                        category_mask.push_back(j);
                    }
                }
            }

           
            std::vector<float> cat(66*candidate_num);//1*65*8400 = 64*8400 + 1*8400
            for(int i=0;i<(64+2);i++)
            {   
                std::copy(data_stride_8+i*stride_8_num*stride_8_num, data_stride_8+(i+1)*stride_8_num*stride_8_num, cat.data()+i*candidate_num ); 
                std::copy(data_stride_16+i*stride_16_num*stride_16_num, data_stride_16+(i+1)*stride_16_num*stride_16_num, cat.data()+i*candidate_num+stride_8_num*stride_8_num ); 
                std::copy(data_stride_32+i*stride_32_num*stride_32_num, data_stride_32+(i+1)*stride_32_num*stride_32_num, cat.data()+i*candidate_num+stride_8_num*stride_8_num+stride_16_num*stride_16_num );         
            }

            //process the candidate xywh begin  
            //tranpose and softmax
            std::vector<float> reshape_box(candidate_num*64);
            tranpose(cat.data(),reshape_box.data(),64,candidate_num );
            candidate_num = match_index.size();
            candicate_num = candidate_num;
            std::vector<float> reshape_boxtmp(candidate_num*64);
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1,  5 + key_point_num*2, candidate_num}, -1, memory::NCHW));

            for (size_t i = 0; i < match_index.size(); i++)
                std::copy(reshape_box.data()+match_index[i]*64,reshape_box.data()+match_index[i]*64+64,reshape_boxtmp.data()+i*64);

            int index = 0;
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                {
                    Softmax(reshape_boxtmp.data()+ 16*index ,16 ) ;
                    index++ ;
                }

            //reshape and tranpose  64*8400 ->8400*64
            std::vector<float> reshape_box2(16*4*candidate_num);
                for(int i=0; i<candidate_num; i++)
                    for(int j=0; j<4; j++)
                        for(int k=0; k<16; k++)
                            reshape_box2[k*4*candidate_num +j*candidate_num +i ] = reshape_boxtmp[i*16*4 + j*16+k ];

            //16个通道 1*1卷积
            std::vector<float> conv(4*candidate_num,0);
            for(int i=0;i<16;i++)
                for(int j=0;j<4*candidate_num;j++)
                    conv[j] = conv[j] +reshape_box2[i*4*candidate_num+j ]*i  ; 

            std::vector<float>  concat(candidate_num*4);
            for(int i=0;i<candidate_num*2;i++)
            {              
                int index = match_index[i];
                if(i>=candidate_num  )
                    index = match_index[i - candidate_num ]+totol_size;
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + posture_add_weight[ index] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );      // add_data[i]-sub_data[i]) ;  
            }
            //process the candidate xywh end  
            //process the candidate keypoint begin  
            std::vector<float> PostureXy_Conf(key_point_num*3*candidate_num);
            
            for(int m=0; m<key_point_num; m++)
            {
                for (size_t j = 0; j <candidate_num ; j++)
                {
                    int index = m*totol_size*2  + match_index[j];
                    PostureXy_Conf[ m*candidate_num*2 + j] =                  ((posture_ptr[index]*2 + posture_add_weight[   match_index[j]] ) * posture_mul_weight[ match_index[j]] ); 
                    PostureXy_Conf[ m*candidate_num*2 + candidate_num + j] =  ((posture_ptr[index+totol_size]*2 + posture_add_weight[ totol_size + match_index[j]] ) * posture_mul_weight[ match_index[j]] ); 
                    // PostureXy_Conf[ m*candidate_num*3 + candidate_num*2 + j] =  sigmoid_x(posture_ptr[index + totol_size*2 ]) ;//最右侧sigmoid
                }            
            }   
            //concat the output
            float * output=output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                output[candidate_num*0 +i] = concat[candidate_num*0 +i]*posture_mul_weight[ match_index[i]];    
                output[candidate_num*1 +i] = concat[candidate_num*1 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*2 +i] = concat[candidate_num*2 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*3 +i] = concat[candidate_num*3 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*4 +i] = sigmoid_x(cat[totol_size*64 + match_index[i] + category_mask[i]*totol_size ]);
            }
            std::memcpy(output+5*candidate_num, PostureXy_Conf.data(), key_point_num*2*candidate_num*sizeof(float));
            return  output0;

        }



        std::vector<std::vector<float>> XYXY2WH(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, int key_point_num,
                        int candicate_num, std::vector<int>& class_mask, float threshold=0.0,float iou_thres=0.8 )
        {
            std::vector<std::vector<float>> output;
            int shape = 5 + key_point_num*2;
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
                    std::vector<float> temp(5 + key_point_num*2);
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

                    for(int j=0;j<key_point_num;j++)
                    {
                        temp[2*j + 5]    = (dest_ptr[shape*i+5+j*2 ]-pad_w)*scale;
                        temp[2*j + 1 + 5]= (dest_ptr[shape*i+5+j*2+1 ]-pad_h)*scale;
                        // temp[3*j + 2 + 5]=  dest_ptr[shape*i+5+j*3+2 ];
                    }
   
                    output.push_back(temp);
            }
            return output ;
        }







#endif
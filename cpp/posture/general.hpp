#ifndef __GENERAL_HPP__
#define __GENERAL_HPP__

#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"  
    
    using namespace glasssix;
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

    inline float de_sigmoid(float x)
    {
        if(x>=1 ||x<0)
            return NAN;
        return static_cast<float> (log( x/(1-x)));
    }

      std::shared_ptr<memory::tensor<float>> Posture_Concat640(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, int key_point_num,float conf,int& candicate_num,std::vector<float>& posture_add_weight,std::vector<float>& posture_mul_weight)
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
            const float* data_stride_8_conf = data_stride_8+stride_8_num*stride_8_num*box_tmp_size;
            for (size_t i = 0; i < stride_8_num*stride_8_num; i++)
                if( data_stride_8_conf[i] >conf  )
                    match_index.push_back(i);
            const float* data_stride_16_conf = data_stride_16+stride_16_num*stride_16_num*box_tmp_size;
            for (size_t i = 0; i < stride_16_num*stride_16_num; i++)
                if( data_stride_16_conf[i] >conf )
                    match_index.push_back(i+stride_8_num*stride_8_num);
            const float* data_stride_32_conf = data_stride_32+stride_32_num*stride_32_num*box_tmp_size;
            for (size_t i = 0; i < stride_32_num*stride_32_num; i++)
                if( data_stride_32_conf[i] >conf  )    
                    match_index.push_back(i+ stride_8_num*stride_8_num + stride_16_num*stride_16_num );
           
            std::vector<float> cat(65*candidate_num);//1*65*8400 = 64*8400 + 1*8400
            for(int i=0;i<65;i++)
            {   
                int j=0;
                for(; j<stride_8_num*stride_8_num; j++)             
                    cat[ i*candidate_num + j] = data_stride_8[i*stride_8_num*stride_8_num + j];
                for(; j<stride_16_num*stride_16_num+stride_8_num*stride_8_num; j++)              
                    cat[ i*candidate_num + j] = data_stride_16[i*stride_16_num*stride_16_num + j- stride_8_num*stride_8_num];                        
                for(; j<stride_32_num*stride_32_num+stride_16_num*stride_16_num+stride_8_num*stride_8_num; j++)
                    cat[ i*candidate_num + j] = data_stride_32[i*stride_32_num*stride_32_num + j-stride_8_num*stride_8_num  -stride_16_num*stride_16_num ];
            }

            //process the candidate xywh begin  
            //tranpose and softmax
            std::vector<float> reshape_box(candidate_num*64);
            tranpose(cat.data(),reshape_box.data(),64,candidate_num );
            candidate_num = match_index.size();
            candicate_num = candidate_num;
            std::vector<float> reshape_boxtmp(candidate_num*64);
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1,  5 + key_point_num*3, candidate_num}, -1, memory::NCHW));

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
                    int index = m*totol_size*3  + match_index[j];
                    PostureXy_Conf[ m*candidate_num*3 + j] =                  ((posture_ptr[index]*2 + posture_add_weight[   match_index[j]] ) * posture_mul_weight[ match_index[j]] ); 
                    PostureXy_Conf[ m*candidate_num*3 + candidate_num + j] =  ((posture_ptr[index+totol_size]*2 + posture_add_weight[ totol_size + match_index[j]] ) * posture_mul_weight[ match_index[j]] ); 
                    PostureXy_Conf[ m*candidate_num*3 + candidate_num*2 + j] =  sigmoid_x(posture_ptr[index + totol_size*2 ]) ;//最右侧sigmoid
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
                output[candidate_num*4 +i] =  sigmoid_x(cat[totol_size*64 +match_index[i]]);
            }
            std::memcpy(output+5*candidate_num, PostureXy_Conf.data(), key_point_num*3*candidate_num*sizeof(float));
            return  output0;

        }

        std::shared_ptr<memory::tensor<float>> Posture_Concat1280(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, int key_point_num,float conf,int& candicate_num,std::vector<float>& posture_add_weight_1280single,std::vector<float>& posture_mul_weight_1280single)
        {
            conf = de_sigmoid(conf);
            int input = 1280;// input_size: 1280*1280, only support single class
            int box_tmp_size = 64;
            int stride_8_num = input / 8;
            int candidate_num= stride_8_num*stride_8_num ;
            int totol_size = stride_8_num*stride_8_num;

            //160 keypoint
            const float *data_stride_8 = outs[0]->cpu_data();
            const float *posture_ptr = outs[1]->cpu_data();

            std::vector<int> match_index;

            const float* data_stride_8_conf = data_stride_8 + stride_8_num*stride_8_num*box_tmp_size;
            for (size_t i = 0; i < stride_8_num*stride_8_num; i++)
                if(data_stride_8_conf[i] >conf  )
                    match_index.push_back(i);

            //tranpose and softmax
            std::vector<float> reshape_box(candidate_num*64);
            tranpose(data_stride_8, reshape_box.data(),64,candidate_num );

            candidate_num = match_index.size();

            candicate_num = candidate_num;
            std::vector<float> reshape_boxtmp(candidate_num*64);
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                (new memory::tensor<float>(std::vector<int>{1, 5 + key_point_num*3, candidate_num}, -1, memory::NCHW));

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
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + posture_add_weight_1280single[ index] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );    
            }
            //process the candidate keypoint begin  
            std::vector<float> PostureXy_Conf(key_point_num*3*candidate_num);
            
            for(int m=0; m<key_point_num; m++)
            {
                for (size_t j = 0; j <candidate_num ; j++)
                {
                    int index = m*totol_size*3  + match_index[j];
                    PostureXy_Conf[ m*candidate_num*3 + j] =                    ((posture_ptr[index]*2 + posture_add_weight_1280single[   match_index[j]] ) * posture_mul_weight_1280single[ match_index[j]] ); 
                    PostureXy_Conf[ m*candidate_num*3 + candidate_num + j] =    ((posture_ptr[index+totol_size]*2 + posture_add_weight_1280single[ totol_size + match_index[j]] ) * posture_mul_weight_1280single[ match_index[j]] ); 
                    PostureXy_Conf[ m*candidate_num*3 + candidate_num*2 + j] =  sigmoid_x(posture_ptr[index + totol_size*2 ]) ;//最右侧sigmoid
                }            
            }   

            //process the candidate keypoint end
            //concat the output
            float * output=output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                output[candidate_num*0 +i] = concat[candidate_num*0 +i]*posture_mul_weight_1280single[ match_index[i]];    
                output[candidate_num*1 +i] = concat[candidate_num*1 +i]*posture_mul_weight_1280single[ match_index[i]];
                output[candidate_num*2 +i] = concat[candidate_num*2 +i]*posture_mul_weight_1280single[ match_index[i]];
                output[candidate_num*3 +i] = concat[candidate_num*3 +i]*posture_mul_weight_1280single[ match_index[i]];
                output[candidate_num*4 +i] = sigmoid_x(data_stride_8[totol_size*64 +match_index[i]]);
            }
            std::memcpy(output+5*candidate_num, PostureXy_Conf.data(), key_point_num*3*candidate_num*sizeof(float));
            return  output0;
        }

        std::vector<std::vector<float>> XYXY2WH(std::shared_ptr<memory::tensor<float>>& net_result, cv::Mat & blob, int pad_h, int pad_w, float scale, int key_point_num,int candicate_num, float threshold=0.4,float iou_thres=0.8 )
        {
            std::vector<std::vector<float>> output;
            int shape = 5 + key_point_num*3;
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
                    std::vector<float> temp(5 + key_point_num*3);
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
                        temp[3*j + 5]    = (dest_ptr[shape*i+5+j*3 ]-pad_w)*scale;
                        temp[3*j + 1 + 5]= (dest_ptr[shape*i+5+j*3+1 ]-pad_h)*scale;
                        temp[3*j + 2 + 5]=  dest_ptr[shape*i+5+j*3+2 ];
                    }
   
                    output.push_back(temp);
            }
            return output ;
        }

#endif
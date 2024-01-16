#ifndef __GENERAL_HPP__
#define __GENERAL_HPP__

#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"  
    
        using namespace glasssix;
        struct slide_pics_params
        {
            std::vector<cv::Mat> imgs;
            std::vector<float> bias;
            std::vector<float> sou_mat_bias;
            bool horizontal=true;
            float ratio=1.f;
            std::vector<int> throw_result_border;
            bool detect=true;
        };

        void border_judgement(std::vector<std::vector<float>>& nms_input,int height,int width,bool horizontal=true, bool border=false)
        {
            if(horizontal) //检测左右两边 给出极大可能是边界数据
            {
                // nms_input

            }         
        }
    
        slide_pics_params Sliding_Cut_Pic(cv::Mat& sou_img )
        {
            std::vector<cv::Mat> mats;
            std::vector<float> bias;
            slide_pics_params return_data;

            int pic_width  = sou_img.cols;
            int pic_height = sou_img.rows;
            bool horizontal = true;
            float short_side;
            float long_side;
            float long_short_ratio;
            if ( pic_width > pic_height)
            {
                short_side = pic_height;
                long_side = pic_width;
            }
            else
            {
                short_side = pic_width;
                long_side = pic_height;
                horizontal = false;
            }
            return_data.horizontal = horizontal; 
            bias.push_back(0);
            // sou_mat_bias.push_back(0);
            long_short_ratio = long_side/short_side;

            float scale_ratio = short_side / 640.f;
            
            if((long_side/640.f) <=1.5) //长边放缩比小于1.5 直接返回原图
            {
                mats.push_back(sou_img);
                return_data.imgs=mats;
                return_data.bias=bias;
                scale_ratio = long_side / 640.f;
                return_data.ratio = 1.f / scale_ratio;
                return_data.throw_result_border.resize(2);
                return_data.throw_result_border[0]=0;
                return_data.throw_result_border[1]=0;
                return_data.detect = false;
                return return_data;
            }
            return_data.ratio = 1.f / scale_ratio;
            //以短边为基准 滑动窗口
        
            cv::Mat temp_pic;
            cv::resize(sou_img, temp_pic, cv::Size(std::round(sou_img.cols / scale_ratio), std::round(sou_img.rows / scale_ratio)), cv::INTER_LINEAR);
        
            //若长宽比小于1.5 只分割两次
        
            if(long_short_ratio<1.3 )
            {

                cv::Mat blob1;
                cv::Mat blob2;
                //长大于宽
                if(horizontal)
                {
                    blob1 = temp_pic(cv::Range(0, 640), cv::Range(0, 640));
                    blob2 = temp_pic(cv::Range(0, 640), cv::Range(temp_pic.cols-640, temp_pic.cols));
                    bias.push_back(temp_pic.cols-640);
                }
                else
                {
                    blob1 = temp_pic(cv::Range(0, 640), cv::Range(0, 640));
                    blob2 = temp_pic(cv::Range(temp_pic.rows-640, temp_pic.rows), cv::Range(0, 640));
                    bias.push_back(temp_pic.rows-640);
                }
            
                mats.push_back(blob1);
                mats.push_back(blob2);
                return_data.imgs=mats;
                return_data.bias=bias;
                return_data.throw_result_border.resize(4);
                return_data.throw_result_border[0]=0;
                return_data.throw_result_border[1]=1;
                return_data.throw_result_border[2]=1;
                return_data.throw_result_border[3]=0;

                return return_data;
            }

            cv::Mat blob1;
            cv::Mat blob2;
            cv::Mat blob3;
            //长大于宽 分割三次
            if(horizontal)
            {
                blob1 = temp_pic(cv::Range(0, 640), cv::Range(0, 640));
                blob2 = temp_pic(cv::Range(0, 640), cv::Range(temp_pic.cols/2-320 ,temp_pic.cols/2+320));
                blob3 = temp_pic(cv::Range(0, 640), cv::Range(temp_pic.cols-640, temp_pic.cols));
                bias.push_back(temp_pic.cols/2-320);
                bias.push_back(temp_pic.cols-640);
            }
            else
            {
                blob1 = temp_pic(cv::Range(0, 640), cv::Range(0, 640));
                blob2 = temp_pic(cv::Range(temp_pic.rows/2-320 ,temp_pic.rows/2+320), cv::Range(0, 640));
                blob3 = temp_pic(cv::Range(temp_pic.rows-640, temp_pic.rows), cv::Range(0, 640));
                bias.push_back(temp_pic.rows/2-320);
                bias.push_back(temp_pic.rows-640);
            }
            mats.push_back(blob1);
            mats.push_back(blob2);
            mats.push_back(blob3);
            return_data.imgs=mats;
            return_data.bias=bias;
                return_data.throw_result_border.resize(6);
                return_data.throw_result_border[0]=0;
                return_data.throw_result_border[1]=1;
                return_data.throw_result_border[2]=1;
                return_data.throw_result_border[3]=1;
                return_data.throw_result_border[4]=1;
                return_data.throw_result_border[5]=0;
            return return_data;

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

        inline float de_sigmoid(float x)
        {
            if(x>=1 ||x<0)
                return NAN;
            return static_cast<float> (log( x/(1-x)));
        }


        std::vector<std::vector<float>> throw_border_result( std::vector<std::vector<float>> & input, bool horizontal, int right, int left, int square_len)
        {
            //x y w h
            std::vector<std::vector<float>>  after_throw;
            int x_y_bias=0;
            if(!horizontal)//竖立图像
            {
                x_y_bias=1;
            }
            for (size_t i = 0; i < input.size(); i++)
            {
                if(input[i][x_y_bias]<square_len*0.02 && right )                         //左侧靠近边界
                {
                    continue;
                }

                if( (  (input[i][2+ x_y_bias] + input[i][x_y_bias])>square_len*0.98 ) && left )                         //右侧靠近边界
                {
                    continue;
                }

                after_throw.push_back(input[i]);        //未靠近边界的图像
            }
            
            return after_throw;

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

        std::vector<std::vector<float>> XYXY2WH(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, int key_point_num,int candicate_num, float threshold=0.0,float iou_thres=0.8 )
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
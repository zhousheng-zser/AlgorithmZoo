#include<vector>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include "computational.hpp"

        using namespace glasssix;
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

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, float threshold=0.7,float iou_thres=0.6 )
        {
              std::vector<std::vector<float>> output;

            int shape =5;
            const int candidate_num=8400;
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

        std::shared_ptr<memory::tensor<float>> Yovo8s_Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, std::vector<float>& posture_add_weight,std::vector<float>& posture_mul_weight  )
        {
            const int candidate_num=8400;
            std::shared_ptr<glasssix::memory::tensor<float>> output0
                            (new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));

            //20 40 80 keypoint
            const float *data80=outs[2]->cpu_data();
            const float *data40=outs[1]->cpu_data();
            const float *data20=outs[0]->cpu_data();

            //concat the 80*40 40*40 20*20 
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
            tranpose(cat.data(),reshape_box.data(),64,candidate_num );

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
            return  output0;
        }

        void init_data(std::vector<float>& posture_add_weight, std::vector<float>& posture_mul_weight)
        {
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

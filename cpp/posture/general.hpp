#ifndef __GENERAL_HPP__
#define __GENERAL_HPP__

#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"  

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

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

        slide_pics_params Sliding_Cut_Pic(cv::Mat& sou_img, float img_size=960 )
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

            float scale_ratio = short_side / img_size;
            
            if((long_side/img_size) <=1.5) //长边放缩比小于1.5 直接返回原图
            {
                mats.push_back(sou_img);
                return_data.imgs=mats;
                return_data.bias=bias;
                scale_ratio = long_side / img_size;
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
                    blob1 = temp_pic(cv::Range(0, img_size), cv::Range(0, img_size));
                    blob2 = temp_pic(cv::Range(0, img_size), cv::Range(temp_pic.cols-img_size, temp_pic.cols));
                    bias.push_back(temp_pic.cols-img_size);
                }
                else
                {
                    blob1 = temp_pic(cv::Range(0, img_size), cv::Range(0, img_size));
                    blob2 = temp_pic(cv::Range(temp_pic.rows-img_size, temp_pic.rows), cv::Range(0, img_size));
                    bias.push_back(temp_pic.rows-img_size);
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
                blob1 = temp_pic(cv::Range(0, img_size), cv::Range(0, img_size));
                blob2 = temp_pic(cv::Range(0, img_size), cv::Range(temp_pic.cols/2-320 ,temp_pic.cols/2+320));
                blob3 = temp_pic(cv::Range(0, img_size), cv::Range(temp_pic.cols-img_size, temp_pic.cols));
                bias.push_back(temp_pic.cols/2-320);
                bias.push_back(temp_pic.cols-img_size);
            }
            else
            {
                blob1 = temp_pic(cv::Range(0, img_size), cv::Range(0, img_size));
                blob2 = temp_pic(cv::Range(temp_pic.rows/2-320 ,temp_pic.rows/2+320), cv::Range(0, img_size));
                blob3 = temp_pic(cv::Range(temp_pic.rows-img_size, temp_pic.rows), cv::Range(0, img_size));
                bias.push_back(temp_pic.rows/2-320);
                bias.push_back(temp_pic.rows-img_size);
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

        std::vector<std::vector<float>> throw_border_result( std::vector<std::vector<float>> & input, bool horizontal, int right, int left, int square_len)
        {
            //x y w h
            std::vector<std::vector<float>>  after_throw;
            int x_y_bias=0;
            if(!horizontal)//竖立图像
                x_y_bias=1;
            for (size_t i = 0; i < input.size(); i++)
            {
                if(input[i][x_y_bias]<square_len*0.02 && right )                         //左侧靠近边界
                    continue;
                if( (  (input[i][2+ x_y_bias] + input[i][x_y_bias])>square_len*0.98 ) && left )                         //右侧靠近边界
                    continue;
                after_throw.push_back(input[i]);        //未靠近边界的图像
            }
            return after_throw;
        }

        std::vector<ObjectInfo> throw_border_resulttest( std::vector<ObjectInfo> & input, bool horizontal, int right, int left, int square_len)
        {
            std::vector<ObjectInfo>  after_throw;
            int x_y_bias=0;
            if(!horizontal)                                                                                     //竖立图像
                x_y_bias=1;
            for (size_t i = 0; i < input.size(); i++)
            {
                if( (horizontal? input[i].x1:input[i].y1 ) <square_len*0.02 && right )                         //左侧靠近边界
                    continue;
                if( (horizontal? input[i].x2:input[i].y2 ) >square_len*0.98  && left )                         //右侧靠近边界
                    continue;
                after_throw.push_back(input[i]);                                                               //未靠近边界的图像
            }
            return after_throw;
        }


        float intersectionOverUnion(const Box& box1, const Box& box2) 
        {
            float x1 = std::max(box1[0], box2[0]);
            float y1 = std::max(box1[1], box2[1]);
            float x2 = std::min(box1[0] + box1[2], box2[0] + box2[2]);
            float y2 = std::min(box1[1] + box1[3], box2[1] + box2[3]);

            if (x1 >= x2 || y1 >= y2) {
                return 0.0f; 
            }

            float intersection_area = (x2 - x1) * (y2 - y1);
            float area1 = box1[2] * box1[3];
            float area2 = box2[2] * box2[3];
            float union_area = area1 + area2 - intersection_area;

            return intersection_area / union_area;
        }

        std::vector<int> object_nms(const std::vector<Box>& boxes, float iou_threshold) 
        {
            std::vector<int> indices(boxes.size());
            for (size_t i = 0; i < boxes.size(); i++)
                indices[i]=i;
            
            std::vector<float> scores;
            for (size_t i = 0; i < boxes.size(); ++i) 
                scores.push_back(boxes[i][4]);

            std::sort(indices.begin(), indices.end(), [&scores](int a, int b) {
                return scores[a] > scores[b];
            });

            std::vector<int> keep;
            while (indices.size() > 0) 
            {
                int idx = indices[0];
                keep.push_back(idx);

                std::vector<int> new_indices;
                for (size_t i = 1; i < indices.size(); ++i) 
                {
                    int cur_idx = indices[i];
                    if (intersectionOverUnion(boxes[idx], boxes[cur_idx]) <= iou_threshold) 
                        new_indices.push_back(cur_idx);          
                }
                indices = new_indices;
            }
            return keep;
        }
        
#endif
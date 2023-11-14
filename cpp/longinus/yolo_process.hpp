#ifndef __YOLO_PROCESS_HPP__
#define __YOLO_PROCESS_HPP__

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <arm_neon.h>
#include <cmath>
    struct boxes_conf
    {
        float top_x;
        float top_y;
        float bot_x;
        float bot_y;
        float conf;
        int category;
    };

    typedef struct Bbox 
    {
        int x;
        int y;
        int w;
        int h;
        float score;
        int category;
    }Bbox;

    struct location_char
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int category;
        float confidence;
    };

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


    static bool sort_score(Bbox box1,Bbox box2) {
        return box1.score > box2.score ? true : false;
    }

    //transpose  1*18*h*w -> 1*3*h*w*6
    void transpose(const float* in, float* out ,int data_num )
    {
        int hw = data_num/(3*6);

        const float* src = in;
        float* dst = out;
        for(int i=0; i<3; i++)
        {
            // src+=( 3*hw );
            // dst+=( 3*hw );
            for (size_t j = 0; j < 6; j++)
            {
                for (size_t k = 0; k < hw; k++)
                {
                    dst[k*6 +j+i*hw*6 ] = src[j*hw+k + i*hw*6];
                }
            }
        }

        int l=0;
    }

    static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    std::vector<std::vector<float>> concat(std::vector<float*>& outs, float conf_thres)
    {

        int img_size = 320;
        int category = 6;
        const float anchors[3][6] = { {4,5, 6,8, 10,12}, {15,19, 23,30, 39,52}, {72,97, 123,164, 209,297} };
        const float stride[3] = { 8.0, 16.0, 32.0 };//80 40 20 ->   30 15 60
        std::vector<std::vector<float>> result;
        for(int n = 0; n < 3; n++)
        {
            int num_grid_x = (int)(img_size / stride[n]);
            int num_grid_y = (int)(img_size / stride[n]);

            int ind = 0;
            float *ptr_out=outs[n];
            for(int q = 0; q < 3; q++)
            {
                const float anchor_w = anchors[n][q * 2];
                const float anchor_h = anchors[n][q * 2 + 1];
                for(int i = 0; i < num_grid_x; i++)
                {
                    for(int j = 0; j < num_grid_y; j++)
                    {
                        float* pdata = ptr_out + ind *  category;
                        float box_score = sigmoid_x(pdata[4]);

                        float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                        float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                        float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                        float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                        std::vector<float> element = {cx, cy, w, h, box_score, sigmoid_x(pdata[5])};
                        result.push_back(element);

                        ind++;
                    }
                }
            }
        }
        return result;
    }

    static std::vector<boxes_conf> xywh2xyxy(std::vector<std::vector<float>>& src, float conf_thres=0.f)
    {
        int index=0;
        std::vector<boxes_conf> res;
        for(auto it: src)
        {
            
            float top_x = it[0] - it[2] / 2;
            float top_y = it[1] - it[3] / 2;
            float bot_x = it[0] + it[2] / 2;
            float bot_y = it[1] + it[3] / 2;
            float conf  = it[4];
            int maxPosition = std::max_element(it.begin()+5, it.end()) - it.begin();
            if(it[maxPosition] * conf > conf_thres)
            {
                boxes_conf temp{};
                temp.top_x = top_x;
                temp.top_y = top_y;
                temp.bot_x = bot_x;
                temp.bot_y = bot_y;
                temp.conf = it[maxPosition] * conf;
                temp.category =  maxPosition - 5;
                res.push_back(temp);              

            }
            index ++;
        }
        return res;
    }

    static float iou(Bbox box1, Bbox box2) 
    {
        int x1 = std::max(box1.x, box2.x);
        int y1 = std::max(box1.y, box2.y);
        int x2 = std::min(box1.x + box1.w, box2.x + box2.w);
        int y2 = std::min(box1.y + box1.h, box2.y + box2.h);
        int w = std::max(0, x2 - x1);
        int h = std::max(0, y2 - y1);
        float over_area = w * h;
        return over_area / (box1.w*box1.h + box2.w*box2.h - over_area);
    }

    static std::vector<Bbox> nms(std::vector<Bbox>&boxes, float threshold)
    {
        std::vector<Bbox>resluts;
        std::sort(boxes.begin(), boxes.end(), sort_score);
        while (boxes.size()> 0) 
        {
            resluts.push_back(boxes[0]);
            int index = 1;
            while (index < boxes.size()) {
                float iou_value = iou(boxes[0], boxes[index]);
                if (iou_value > threshold) {
                    boxes.erase(boxes.begin() + index);
                }
                else {
                    index++;
                }
            }
            boxes.erase(boxes.begin());
        }
        return  resluts;
    }

    static std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh,float ratio,int pad_h,int pad_w)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for(auto const &it: src)
            {
         
                // int c = max_wh * it.conf;
                Bbox temp;
                temp.x      = static_cast<double>(it.top_x-pad_w )*ratio;
                temp.y      = static_cast<double>(it.top_y-pad_h)*ratio;
                temp.w  = static_cast<double>(it.bot_x - it.top_x)*ratio;
                temp.h  = static_cast<double>(it.bot_y - it.top_y)*ratio;
                temp.score=it.conf;
                temp.category = it.category;
                // std::cout<<it.top_x<<" "<< temp.x<<std::endl;
                //if (temp.category == 0 || temp.category == 4)
                //{
                    boxes.push_back(temp);
                //}
            }
            return boxes;
        }

		/**
		 * @fun non_max_suppression
		 * @param prediction, conf_thres, iou_thres
		 * @return std::vector(boxes, classes)
		 * @details Non-Maximum Suppression (NMS) on inference results
		 */
		static std::vector<location_char> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio,int pad_h,int pad_w)
        {
            // std::cout<<"nms inpu size "<<prediction.size()<<std::endl;
            //std::cout<<ratio<<std::endl;
            auto compute_box = xywh2xyxy(prediction, conf_thres);  

            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes= computeNmsInput(compute_box, max_wh,ratio,pad_h,pad_w );//此处做分类处理，因为有四类 而非以前的单类

            // std::cout<<"ori_size:"<<boxes.size()<<std::endl;
            std::vector<Bbox> class_work;
            std::vector<Bbox> class_other;
            for (auto &box:boxes)
            {
                class_work.emplace_back(box);
            }
            auto bboxes_work=nms(class_work, iou_thres);
            std::vector<location_char> output;

            for (auto it : bboxes_work)
            {   
                location_char temp;
                temp.x1=it.x;
                temp.x2=it.x+it.w;
                temp.y1=it.y;
                temp.y2=it.y+it.h;
                temp.category = it.category;
                temp.confidence=it.score;
                output.emplace_back(temp);
            }
            return output;
        }

#if defined(BUILD_RV1106) 
    void MatrixMul(float32_t* matrixA, float32_t* matrixB, float32_t* result, int L,int M,int N){ // 1 208 14
        for (int i = 0; i < N; ++i) 
        {
            float32x4_t acc = vdupq_n_f32(0.0f);
            for (int j = 0; j < M; j += 4) 
            {
                float32x4_t a = vld1q_f32(&matrixB[i * M+ j]);
                float32x4_t b = vld1q_f32(&matrixA[j]);
                acc = vmlaq_f32(acc, a, b);
            }
            float32_t temp[4];
            vst1q_f32(temp, acc);
            result[i] = temp[0] + temp[1] + temp[2] + temp[3];
        }
    }
#endif
#endif // !__YOLO_PROCESS_HPP__

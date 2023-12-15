#include <opencv2/opencv.hpp>
#include "../posture/box_info.hpp"
// #include <opencv2/opencv.hpp>
#include "Excalibur/pipeline.hpp"

using namespace glasssix;

    
  

    void tranpose(const float* sou,
                        float* dest,int sourows,int soucols)
    {
        for(int i=0;i< sourows;i++)
        {
            for(int j=0;j< soucols;j++)
            {
                dest[j*sourows+i]=sou[ i * soucols + j];    
            }
        }
    }



    static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    inline float de_sigmoid(float x)
    {
        if(x>=1 ||x<0)
            return NAN;
        return static_cast<float> (log( x/(1-x)));
    }


    struct PostureInfo
    {
        PostureInfo(posture::box_info& b_info) {
            x1 = b_info.x1();
            x2 = b_info.x2();
            y1 = b_info.y1();
            y2 = b_info.y2();
            score = b_info.score();
            category = b_info.category();

            auto key_points = b_info.key_points();
            for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
				std::pair<cv::Point, float> key_p;
				key_p.first.x = key_points[i * 3];
                key_p.first.y = key_points[i * 3 + 1];
                key_p.second = key_points[i * 3 + 2];
                Kpoints.push_back(key_p);
            }
            //   std::vector<std::pair<cv::Point, float>> Kpoints;
        }

        cv::Rect get_rect() {
            return cv::Rect{
                cv::Point(std::round(x1), std::round(y1)),
                cv::Point(std::round(x2), std::round(y2)) };
        }

		std::int32_t x1;
		std::int32_t y1;
		std::int32_t x2;
		std::int32_t y2;
		float score;
		int category;
        std::vector<std::pair<cv::Point, float>> Kpoints;
    };




    struct safe_crop_rect 
    {
        int x1;
        int x2;
        int y1;
        int y2;
        safe_crop_rect(int x11,int x22,int y11,int y22,int width,int height)
        {
            x1 = x11>0 ? x11:0;
            x2 = x22>0 ? x22:0;
            y1 = y11>0 ? y11:0;
            y2 = y22>0 ? y22:0;

            x1 = x1 <width? x1 : width;
            x2 = x2 <width? x2 : width;
            y1 = y1 <height? y1 : height;
            y2 = y2 <height? y2 : height;
        }

    };

    struct Cigrate_box
    {
        int m_left;
        int m_top;
        int m_width;
        int m_height;
    
        Cigrate_box() {}
        Cigrate_box(int x1, int y1, int x2, int y2) 
        {
            m_left = x1;
            m_top = y1;
            m_width = x2 - x1;
            m_height = y2 - y1;
        }

        int area()
        {
            return m_width*m_height;
        }

    };


    bool is_filterated(Cigrate_box & head, Cigrate_box & cigrate )
    {
        return head.area()<cigrate.area();
    }
    
    float IOU_compute(const Cigrate_box b1, const Cigrate_box b2)
    {

        
        float w = std::max(b1.m_left+b1.m_width,b2.m_left+b2.m_width  ) - std::min(b1.m_left,b2.m_left);
        float h = std::max(b1.m_top+b1.m_height,b2.m_top+b2.m_height  ) - std::min(b1.m_top,b2.m_top);
        float ww = b1.m_width +b2.m_width;
        float hh = b1.m_height +b2.m_height;
        // float w = std::max(std::min((b1.m_left + b1.m_width), (b2.m_left + b2.m_width)) - std::max(b1.m_left, b2.m_left), 0);
        // float h = std::max(std::min((b1.m_top + b1.m_height), (b2.m_top + b2.m_height)) - std::max(b1.m_top, b2.m_top), 0);
        if( ww<w || hh<h)
            return 0; 
        else
            return (ww-w)*(hh-h)/static_cast<float>(b1.m_width*b1.m_height);
    }

    struct Smoke_Point
    {
        int x1;
        int x2;
        int y1;
        int y2;
        float score;
        bool quality_is_ok;
        std::vector<std::pair<cv::Point,float>> nose_eye_ear;//left right
        std::vector<std::pair<cv::Point,float>> shoulder_elbow_wrist;//left right

        Smoke_Point(int x11,int y11, int x22,int y22,float score, std::vector<std::pair<cv::Point,float>>&PersonKpoints )
        {
            x1 = x11;
            y1 = y11;
            x2 = x22;
            y2 = y22;
            score = score;
            int key_point_size = PersonKpoints.size();

            nose_eye_ear.resize(5);
            for (size_t i = 0; i < 5; i++) 
                nose_eye_ear[i]=PersonKpoints[i];

            shoulder_elbow_wrist.resize(5);
            shoulder_elbow_wrist[0]=PersonKpoints[0];

            for (size_t i = 5; i < 9; i++)        
                shoulder_elbow_wrist[i-4]=PersonKpoints[i];

        }

        bool is_detect()
        {
            int count = 0;
            for (size_t i = 0; i < 3; i++)
                if( nose_eye_ear[i].second > 0.8f  )
                    count++;
            return  (count > 1 );
        }

        std::tuple<float,float,float> get_height(std::vector<std::pair<cv::Point,float>>& point)
        {
            float y_min = 10000000.f;
            float y_max = 0.f;
            for(auto var : point)
            {
                if(y_min> var.first.y )
                    y_min=var.first.y;
                if(y_max< var.first.y )
                    y_max=var.first.y;
            }
            return  {y_min , y_max, y_max-y_min};
        }

        std::tuple<float,float,float> get_width(std::vector<std::pair<cv::Point,float>>& point)
        {
            float x_min = 10000000.f;
            float x_max = 0.f;
            for(auto var : point)
            {
                if(x_min> var.first.x )
                    x_min=var.first.x;
                if(x_max< var.first.x )
                    x_max=var.first.x;
            }
            return  {x_min , x_max ,x_max-x_min};
        }

        safe_crop_rect  get_head_area(int widths,int heights)
        {
            float head_x1;
            float head_x2;
            float head_y1;
            float head_y2;
            float width; 
            float height;
            std::tie(head_x1, head_x2, width) =  get_width(nose_eye_ear);
            std::tie(head_y1, head_y2, height) = get_height(nose_eye_ear);
            head_x1 = nose_eye_ear[0].first.x - width/3.5;
            head_x2 = nose_eye_ear[0].first.x + width/3.5;
            head_y1 = head_y2 ;
            head_y2 = head_y2 +   height*2.5;

            safe_crop_rect rect(head_x1,head_x2,head_y1,head_y2,widths,heights );
            return rect;
        }

        safe_crop_rect get_upper_body_area(int width,int height)
        {
            float upper_body_x1;
            float upper_body_x2;
            float upper_body_y1;
            float upper_body_y2;
            float heights;
            upper_body_x1 = x1;
            upper_body_x2 = x2;
            std::tie(upper_body_y1, upper_body_y2, heights) = get_height(shoulder_elbow_wrist);
            upper_body_y1-=20;
            upper_body_y2+=20;
            safe_crop_rect rect(upper_body_x1,upper_body_x2,upper_body_y1,upper_body_y2,width,height);

            return rect;
        }   
};



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


         std::shared_ptr<memory::tensor<float>> Yovo8se_Concat_4B(std::vector<std::shared_ptr<memory::tensor<float>>>& outs,float conf,int& candicate_num ,std::vector<float>& posture_add_weight,std::vector<float>& posture_mul_weight)
        {
            conf = de_sigmoid(conf);
            int input_size = 640;
            int candidate_num=34000;
            int class_num = 65;        
            int stride_num4 = input_size/4;
            int stride_num8 = input_size/8;
            int stride_num16 = input_size/16;
            int stride_num32 = input_size/32;

            //20 40 80 
            const float *data160=outs[3]->cpu_data();
            const float *data80=outs[2]->cpu_data();
            const float *data40=outs[1]->cpu_data();
            const float *data20=outs[0]->cpu_data();

            std::vector<int> match_index;
            const float* data160_conf = data160+stride_num4*stride_num4*64;
            for (size_t i = 0; i < stride_num4*stride_num4; i++)
                if( data160_conf[i] >conf  )
                    match_index.push_back(i);

            const float* dat80_conf = data80+stride_num8*stride_num8*64;
            for (size_t i = 0; i < stride_num8*stride_num8; i++)
                if( dat80_conf[i] >conf  )
                    match_index.push_back(i+stride_num4*stride_num4);

            const float* data40_conf = data40+stride_num16*stride_num16*64;
            for (size_t i = 0; i < stride_num16*stride_num16; i++)
                if( data40_conf[i] >conf )
                    match_index.push_back(i+stride_num4*stride_num4+stride_num8*stride_num8);

            const float* data20_conf = data20+stride_num32*stride_num32*64;
            for (size_t i = 0; i < stride_num32*stride_num32; i++)
                if( data20_conf[i] >conf  )
                    match_index.push_back(i+stride_num4*stride_num4+stride_num8*stride_num8+stride_num16*stride_num16);

            // std::cout<<"var:\n";
            // for(auto var : match_index)
            // {
            //     std::cout<<var<<"\t";
            // }
            // std::cout<<"\n";
            
            //concat the 80*40 40*40 20*20 
            std::vector<float> cat(65*candidate_num);//1*65*8400 = 64*8400 + 1*8400
            for(int i=0;i<65;i++)
            {   
                int j=0;
                for(; j<stride_num4*stride_num4; j++)
                    cat[ i*candidate_num + j] = data160[i*stride_num4*stride_num4 + j];

                for(; j<stride_num4*stride_num4+stride_num8*stride_num8; j++)
                    cat[ i*candidate_num + j] = data80[i*stride_num8*stride_num8 + j - stride_num4*stride_num4];

                for(; j<stride_num8*stride_num8+stride_num16*stride_num16 + stride_num4*stride_num4; j++)
                    cat[ i*candidate_num + j] = data40[i*stride_num16*stride_num16 + j - stride_num8*stride_num8 - stride_num4*stride_num4];     

                for(; j<stride_num8*stride_num8+stride_num16*stride_num16+stride_num4*stride_num4+stride_num32*stride_num32; j++)
                    cat[ i*candidate_num + j] = data20[i*stride_num32*stride_num32 + j - stride_num8*stride_num8 - stride_num16*stride_num16 - stride_num4*stride_num4 ];
            }

            //process the candidate xywh begin  
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
            {
                for(int j=0; j<4; j++)
                {
                    Softmax(reshape_boxtmp.data()+ 16*index ,16 ) ;
                    index++ ;
                }
            }

            std::vector<float> reshape_box2(16*4*candidate_num);
            for(int i=0; i<candidate_num; i++)
                for(int j=0; j<4; j++)
                    for(int k=0; k<16; k++)
                        reshape_box2[k*4*candidate_num +j*candidate_num +i ] = reshape_boxtmp[i*16*4 + j*16+k ];

            std::vector<float> conv(4*candidate_num,0);
            
            for(int i=0;i<16;i++)
                for(int j=0;j<4*candidate_num;j++)
                {
                    int location = 4*candidate_num;
                    conv[j] = conv[j] +reshape_box2[i*location+j ]*i  ; 
                }

            std::vector<float>  concat(candidate_num*4);
            for(int i=0;i<candidate_num*2;i++)
            {              
                int index = match_index[i];
                if(i>=candidate_num  )
                    index = match_index[i - candidate_num ]+34000;                
                concat[i]                 = (conv[i+candidate_num*2] - conv[i] )/2.f + posture_add_weight[ index] + 0.5;     
                concat[i+candidate_num*2] = (conv[i+candidate_num*2] + conv[i] );      // add_data[i]-sub_data[i]) ;  
            }

            //concat the output
            float * output = output0->mutable_cpu_data();
            for(int i=0;i<candidate_num;i++)
            {
                output[candidate_num*0 +i] = concat[candidate_num*0 +i]*posture_mul_weight[ match_index[i]];    
                output[candidate_num*1 +i] = concat[candidate_num*1 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*2 +i] = concat[candidate_num*2 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*3 +i] = concat[candidate_num*3 +i]*posture_mul_weight[ match_index[i]];
                output[candidate_num*4 +i] = sigmoid_x(cat[34000*64 +match_index[i]]);

            }          
            return  output0;

        }


        


        std::vector<std::vector<float>> smoke_post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, int candicate_num, float threshold=0.45,float iou_thres=0.6 )
        {
            std::vector<std::vector<float>> output;

            int shape =5;
            const int candidate_num = candicate_num;
            // const int candidate_num = 34000;
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
                // if(dest_ptr[shape*i+4]>threshold)
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
                        // indices_body.push_back(i);
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
            return output;
        }
#include <opencv2/opencv.hpp>
// #include "../posture/box_info.hpp"
// #include <opencv2/opencv.hpp>
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"
#include <ctime>

using namespace glasssix;

    struct time_sign
    {
        bool first_init = true;
        time_t first_alarm_time;
    };

    struct Rectangle 
    {
        int x1, y1, x2, y2;

        int real_time_x1, real_time_y1, real_time_x2, real_time_y2;

        float score;

        Rectangle()
        {}

        Rectangle(int x1_, int x2_,int y1_,int y2_, float score_):x1(x1_), x2(x2_), y1(y1_), y2(y2_), score(score_)
        {}

        void refresh()
        {
            real_time_x1 = x1;
            real_time_y1 = y1;
            real_time_x2 = x2;
            real_time_y2 = y2;
        }

        void refresh(int x11, int y11,int x22,int y22)
        {
            real_time_x1 = x11;
            real_time_y1 = y11;
            real_time_x2 = x22;
            real_time_y2 = y22;
        }

        bool is_invalid_rect()
        {
            return (x1==0&&x2==0&&y1==0&&y2==0) ;
        }
    };

    struct Quadrilateral
    {
        int x1, y1, x2, y2, x3, y3, x4, y4;
    };

       static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    void tranpose(const float* sou, float* dest, int sourows, int soucols)
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
            data[i] =  data[i] / L2_Sum ;
    }

    inline float de_sigmoid(float x)
    {
        if(x>=1 ||x<0)
            return NAN;
        return static_cast<float> (log( x/(1-x)));
    }

    std::pair<Quadrilateral,Quadrilateral> get_initial_quadrilateral( Rectangle& RectLeft, Rectangle& RectCentre, Rectangle& RectRight )
    {      

        Quadrilateral quadrilateral_left  ;
        Quadrilateral quadrilateral_right ;

        int centre_up_x = ( RectCentre.x1 + RectCentre.x2 )/2;
        int centre_up_y =   RectCentre.y1 ;
        int centre_down_left_x = RectCentre.x1;
        int centre_down_left_y = RectCentre.y2;
        int centre_down_right_x = RectCentre.x2;
        int centre_down_right_y = RectCentre.y2;

        int left_down_x = ( RectLeft.x1 + RectLeft.x2 )/2;
        int left_down_y = RectLeft.y2;

        int right_down_x =  ( RectRight.x1 + RectRight.x2 )/2;
        int right_down_y = RectRight.y2;

        quadrilateral_left.x1 = RectLeft.x2;
        quadrilateral_left.y1 = RectLeft.y1;

        quadrilateral_left.x2 = centre_up_x;
        quadrilateral_left.y2 = centre_up_y;

        quadrilateral_left.x3 = RectCentre.x1;
        quadrilateral_left.y3 = RectCentre.y2;

        quadrilateral_left.x4 = left_down_x;
        quadrilateral_left.y4 = left_down_y;

        quadrilateral_right.x1 = centre_up_x;
        quadrilateral_right.y1 = centre_up_y;

        quadrilateral_right.x2 = RectRight.x1;
        quadrilateral_right.y2 = RectRight.y1;

        quadrilateral_right.x3 = right_down_x;
        quadrilateral_right.y3 = right_down_y;

        quadrilateral_right.x4 = RectCentre.x2;
        quadrilateral_right.y4 = RectCentre.y2;

        return std::make_pair(quadrilateral_left,quadrilateral_right);
    }

    float get_distance_between_Rectangle(const Rectangle&R1, const Rectangle& R2, bool real_time = true)
    {
        if(real_time)
            return  0.5 * sqrt( (R1.real_time_x1-R2.real_time_x1) * (R1.real_time_x2-R2.real_time_x2) + (R1.real_time_y1-R2.real_time_y1) * (R1.real_time_y2-R2.real_time_y2)) ;
        else
            return  0.5 * sqrt( (R1.x1-R2.x1) * (R1.x2-R2.x2) + (R1.y1-R2.y1) * (R1.y2-R2.y2)) ;
    }

    int calculate_distance(Rectangle rect1, Rectangle rect2) 
    {
        int distance = std::min(std::abs(rect1.x2 - rect2.x1), std::abs(rect2.x2 - rect1.x1));
        return distance;
    }

    std::pair<Rectangle, Rectangle> find_nearest_rectangles(std::vector<Rectangle> rectangles, Rectangle target_rect) 
    {
        Rectangle left_nearest (0, 0, 0, 0,0);
        Rectangle right_nearest (0, 0, 0, 0,0);
        int left_distance = INT_MAX;
        int right_distance = INT_MAX;

        for (auto rect : rectangles) {
            if (!(rect.x1 == target_rect.x1 && rect.y1 == target_rect.y1 && rect.x2 == target_rect.x2 && rect.y2 == target_rect.y2)) {
                int distance = calculate_distance(rect, target_rect);
                if (rect.x2 <= target_rect.x2 && distance <= left_distance) {
                    left_nearest = rect;
                    left_distance = distance;
                } else if (rect.x2 >= target_rect.x2 && distance <= right_distance) {
                    right_nearest = rect;
                    right_distance = distance;
                }
            }
        }

        Rectangle null_rectangle (0, 0, 0, 0,0);
        
        if (left_nearest.x1 == 0 && left_nearest.y1 == 0 && left_nearest.x2 == 0 && left_nearest.y2 == 0 &&
            right_nearest.x1 == 0 && right_nearest.y1 == 0 && right_nearest.x2 == 0 && right_nearest.y2 == 0) {
            return std::make_pair(null_rectangle,null_rectangle);  // 左右两边都没有符合条件的矩形框，返回 {0, 0, 0, 0}, {0, 0, 0, 0}
        } else if (left_nearest.x1 == 0 && left_nearest.y1 == 0 && left_nearest.x2 == 0 && left_nearest.y2 == 0) {
            return std::make_pair(null_rectangle, right_nearest);  // 左边没有符合条件的矩形框，返回 {0, 0, 0, 0} 和右边最近的矩形框
        } else if (right_nearest.x1 == 0 && right_nearest.y1 == 0 && right_nearest.x2 == 0 && right_nearest.y2 == 0) {
            return std::make_pair(left_nearest, null_rectangle);  // 右边没有符合条件的矩形框，返回左边最近的矩形框和 {0, 0, 0, 0}
        } else {
            return std::make_pair(left_nearest, right_nearest);  // 左右两边都有符合条件的矩形框，返回左右两边最近的矩形框
        }
    }

    void Quadrilateral_scale( Quadrilateral& quadrilateral ,float scale_ratio,bool left_base=true)
    {
        float scale_of_side = sqrt(scale_ratio);
         int bottomLeft_x,bottomLeft_y;
        if(left_base)
        {
            bottomLeft_x = quadrilateral.x4;
            bottomLeft_y = quadrilateral.y4;
        }
        else
        {
            bottomLeft_x = quadrilateral.x3;
            bottomLeft_y = quadrilateral.y3;
        }
        //以bottomLeft_x，bottomLeft_y 为原点计算得到新的相对坐标

        quadrilateral.x1 = quadrilateral.x1 - bottomLeft_x;
        quadrilateral.y1 = quadrilateral.y1 - bottomLeft_y;

        quadrilateral.x2 = quadrilateral.x2 - bottomLeft_x;
        quadrilateral.y2 = quadrilateral.y2 - bottomLeft_y;

        quadrilateral.x3 = quadrilateral.x3 - bottomLeft_x;
        quadrilateral.y3 = quadrilateral.y3 - bottomLeft_y;

        quadrilateral.x4 = quadrilateral.x4 - bottomLeft_x;
        quadrilateral.y4 = quadrilateral.y4 - bottomLeft_y;

        //获取放缩后的新坐标

        quadrilateral.x1 = quadrilateral.x1*scale_of_side + bottomLeft_x;
        quadrilateral.y1 = quadrilateral.y1*scale_of_side + bottomLeft_y;

        quadrilateral.x2 = quadrilateral.x2*scale_of_side + bottomLeft_x;
        quadrilateral.y2 = quadrilateral.y2*scale_of_side + bottomLeft_y;

        quadrilateral.x3 = quadrilateral.x3*scale_of_side + bottomLeft_x;
        quadrilateral.y3 = quadrilateral.y3*scale_of_side + bottomLeft_y;

        quadrilateral.x4 = quadrilateral.x4*scale_of_side + bottomLeft_x;
        quadrilateral.y4 = quadrilateral.y4*scale_of_side + bottomLeft_y;

    }

    int get_match_id( std::map<int, Rectangle>& librarys, Rectangle Rect, float ratio = 0.2 )
    {      
        std::map<int, Rectangle> ::iterator it;
        int min_distance =  (1 << 30);
        int id=-1;
        for(it=librarys.begin(); it != librarys.end();  it++ )
        {   
            Rectangle library_box = librarys[it->first] ;
            float distance = get_distance_between_Rectangle(library_box, Rect);
            
            //获取到最小的id值
            if( distance < min_distance )
            {   
                min_distance  = distance;
                id = it->first ; 
            }
        }

        if( min_distance > abs(Rect.x2-Rect.x1)*ratio )
        {
            id = -1;
        }
        return id;
    }

 

    std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, int& pad_h, int& pad_w,  cv::Size input_shape = cv::Size(640, 640) )
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

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>>& net_result, int pad_h, int pad_w, float scale, int num,float threshold=0.0,float iou_thres=0.6 )
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
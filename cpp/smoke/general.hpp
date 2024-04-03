#include <opencv2/opencv.hpp>
#include "../posture/box_info.hpp"
// #include <opencv2/opencv.hpp>
#include "Excalibur/pipeline.hpp"

using namespace glasssix;

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

        //返回手腕到嘴部距离是否小于检测框体阈值  如果小于则返回 true
        bool is_distance_of_centre_and_wrist_lessthan_detect_box_threhold (std::vector<std::pair<cv::Point, float>>& wrists_point, float box_max_length )
        {
            int centre_x = (x1+x2)/2;
            int centre_y = (y1+y2)/2;
            float distance1 = (wrists_point[0].first.x-centre_x)*(wrists_point[0].first.x-centre_x) +(wrists_point[0].first.y-centre_y)*(wrists_point[0].first.y -centre_y);
            float distance2 = (wrists_point[1].first.x-centre_x)*(wrists_point[1].first.x-centre_x) +(wrists_point[1].first.y-centre_y)*(wrists_point[1].first.y -centre_y);
            return (box_max_length * box_max_length*0.0484) > std::min(distance1,distance2);
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
        return (head.area()<cigrate.area()) || (cigrate.m_width<10&&cigrate.m_height<10)   ;
    }
    
    float IOU_compute(const Cigrate_box& b1, const Cigrate_box& b2)
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
        std::vector<std::pair<cv::Point,float>> wrists;
        std::vector<std::pair<cv::Point,float>> nose_eye;
        std::vector<std::pair<cv::Point,float>> nose_eye_ear;//left right
        std::vector<std::pair<cv::Point,float>> nose_shoulder_elbow_wrist;//left right

        Smoke_Point(int x11,int y11, int x22,int y22,float score, std::vector<std::pair<cv::Point,float>>& PersonKpoints )
        {
            x1 = x11;
            y1 = y11;
            x2 = x22;
            y2 = y22;
            score = score;
            int key_point_size = PersonKpoints.size();

            nose_eye.resize(3);
            for (size_t i = 0; i < 3; i++) 
                nose_eye[i]=PersonKpoints[i];

            nose_eye_ear.resize(5);
            for (size_t i = 0; i < 5; i++) 
                nose_eye_ear[i]=PersonKpoints[i];

            nose_shoulder_elbow_wrist.resize(5);
            nose_shoulder_elbow_wrist[0]=PersonKpoints[0];

            for (size_t i = 5; i < 9; i++)        
                nose_shoulder_elbow_wrist[i-4]=PersonKpoints[i];
            
            for (size_t i = 9; i < 11; i++)
                wrists.push_back(PersonKpoints[i]);
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
            std::tie(head_x1, head_x2, width) =  get_width(  nose_eye_ear);
            std::tie(head_y1, head_y2, height) = get_height(nose_eye);
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
            std::tie(upper_body_y1, upper_body_y2, heights) = get_height(nose_shoulder_elbow_wrist);
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

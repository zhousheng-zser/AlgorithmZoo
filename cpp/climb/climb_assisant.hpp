#include <opencv2/opencv.hpp>
#include "../posture/box_info.hpp"
// #include <opencv2/opencv.hpp>
#include "Excalibur/pipeline.hpp"

using namespace glasssix;

    

    struct PostureInfo
    {
        PostureInfo(posture::box_info& b_info) 
        {
            bias = 0;
          
            x1 = b_info.x1();
            x2 = b_info.x2();
            y1 = b_info.y1();
            y2 = b_info.y2();
            score = b_info.score();
            category = b_info.category();

            auto key_points = b_info.key_points();
            if((key_points.size() / 3)==12 )
                bias=4;

            for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
				std::pair<cv::Point, float> key_p;
				key_p.first.x = key_points[i * 3];
                key_p.first.y = key_points[i * 3 + 1];
                key_p.second = key_points[i * 3 + 2];
                Kpoints.push_back(key_p);
            }
           
            std::array<float,4> hands_shoulds_index = {5,6,9,10};
            std::array<float,2> roots_index = {15,16};

            for (size_t i = 0; i < hands_shoulds_index.size(); i++)
            {
             
                    int index = hands_shoulds_index[i]-bias;
                    std::pair<cv::Point, float> key_p;
                    key_p.first.x = key_points[index * 3];
                    key_p.first.y = key_points[index * 3 + 1];
                    key_p.second =  key_points[index * 3 + 2];
                    hands_shoulders.push_back(key_p);
            }

            for (size_t i = 0; i < roots_index.size(); i++)
            {
                    int index = roots_index[i];
                    std::pair<cv::Point, float> key_p;
                    key_p.first.x = key_points[index * 3];
                    key_p.first.y = key_points[index * 3 + 1];
                    key_p.second =  key_points[index * 3 + 2];
                    roots.push_back(key_p);
            }

            
        }

        cv::Rect get_rect() {
            return cv::Rect{
                cv::Point(std::round(x1), std::round(y1)),
                cv::Point(std::round(x2), std::round(y2)) };
        }

        bool is_climb_posture(int pic_height,int pic_width)
        {

            // std::cout<<"pic_height: "<<pic_height<<"  pic_width: "<<pic_width<<std::endl;
            if( is_exist_negative_point(pic_height,pic_width) )
                return false;

            // std::cout<<"\n";
            bool hands_up_shoulder =  (hands_shoulders[0].first.y  > hands_shoulders[2].first.y) ||  (hands_shoulders[1].first.y  > hands_shoulders[3].first.y);//   (std::max(hands_shoulders[2].first.y , hands_shoulders[3].first.y) - std::min(hands_shoulders[0].first.y ,hands_shoulders[1].first.y) )>0 ;
            bool root_distance = abs(roots[0].first.y - roots[1].first.y)> ((y2-y1)*0.08 ) ;
            // std::cout<<abs(roots[0].first.y - roots[1].first.y)<<" "<<((y1-y1)*0.03 ) <<std::endl;
            // std::cout<<"root_distance: "<<root_distance<<std::endl;
            if(hands_up_shoulder&&root_distance)
            {
                // std::cout<<" hands"<<std::endl;
                // std::cout<<hands_shoulders[0].first.y<<" "<<hands_shoulders[2].first.y<<" "<<hands_shoulders[1].first.y<<" "<<hands_shoulders[3].first.y<<std::endl;
                // std::cout<<" root "<<std::endl;
                // std::cout<<roots[0].first.y <<" "<<roots[1].first.y <<std::endl;
            
            }

            return hands_up_shoulder&&root_distance;
        }

        bool not_in_pic(int pic_height,int pic_width,int x,int y)
        {
            // std::cout<<"pic_height: "<<pic_height<<"  pic_width: "<<pic_width<<" x: "<<x<<" y: "<<y<<std::endl;
            if(x<0 || x > pic_width || y<0 ||y>pic_height)
                return true;
            return false;
            
        }

        bool is_exist_negative_point(int pic_height,int pic_width)
        {
            // std::cout<<y1<<" -box- "<<y2<<std::endl;
            bool state=false;
            // std::cout<<" is_exist_negative_point\n";
            for(auto var : hands_shoulders)
            {
                // std::cout<<var.first.x<<" "<<var.first.y<<std::endl;
                if( not_in_pic(pic_height,pic_width,var.first.x,var.first.y) )
                {
                    // std::cout<< " state: "<<state<<std::endl;
                    state=true;
                   
                }
            }

            for(auto var : roots)
            {
                // std::cout<<var.first.x<<" "<<var.first.y<<std::endl;
                if( not_in_pic(pic_height,pic_width,var.first.x,var.first.y) )
                {
                    // std::cout<< " state: "<<state<<std::endl;
                    state=true;                 
                }
            }        
            // std::cout<< " state: "<<state<<std::endl;
            return state;
        }

		std::int32_t x1;
		std::int32_t y1;
		std::int32_t x2;
		std::int32_t y2;
		float score;
		int category;
        int bias;
        std::vector<std::pair<cv::Point, float>> Kpoints;
        std::vector<std::pair<cv::Point, float>> hands_shoulders;
        std::vector<std::pair<cv::Point, float>> roots;

    };

        std::vector<float> get_human_lowest_point(std::vector<float>& body_point)
        {
            std::vector<float> out(2);
            float offset=0.03f;
            float  x_top_left  = body_point[0];
            float  y_top_left  = body_point[1];
            float  x_low_right = body_point[2];
            float  y_low_right = body_point[3];
            y_low_right = y_low_right - offset * (y_low_right - y_top_left);
            
            out[0] = (x_top_left + x_low_right ) / 2.f;
            out[1] = y_low_right;
            return out;

        }


    std::vector<int> in_region( std::vector<std::vector<float>>& nms_result,std::vector<cv::Point>&contours  )
    {
        
        std::vector<int> in_region_labels(nms_result.size());
        for(int i=0; i<nms_result.size(); i++)
        {       
            std::vector<float> Human_lowest_point=get_human_lowest_point(nms_result[i]);
            float x=Human_lowest_point[0];
            float y=Human_lowest_point[1];

            // std::cout<<x<<" "<<y<<std::endl;
            in_region_labels[i]=pointPolygonTest(contours, cv::Point2f(x, y),false)>0?1:0;

        }

        return in_region_labels;
    }

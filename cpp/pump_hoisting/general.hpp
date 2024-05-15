#include <opencv2/opencv.hpp>
// #include "../posture/box_info.hpp"
// #include <opencv2/opencv.hpp>
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"
#include <ctime>
#include <YoloFamily/Yolo_wrapper.hpp>

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

        float get_y2()
        {
            return y2;
        }

        float get_rt_y2()
        {
            return real_time_y2;
        }

        float get_centre_x ()
        {
            return (x1+x2)/2;
        }

        float get_centre_y ()
        {
            return (y1+y2)/2;
        }

        float get_centre_rt_x ()
        {
            return (real_time_x1+real_time_x2)/2;
        }

        float get_centre_rt_y()
        {
            return (real_time_y1+real_time_y2)/2;
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

    float get_left_down_corner_distance_between_Rectangle(const Rectangle&R1, const Rectangle& R2, bool real_time = true)
    {
        if(real_time)
            return   abs(R1.get_rt_y2()-R2.get_rt_y2()); 
        else
            return   abs(R1.get_y2()-R2.get_y2()); 
    }

    float get_left_top_corner_distance_between_Rectangle(const Rectangle&R1, const Rectangle& R2, bool real_time = true)
    {
        if(real_time)
            return   abs(R1.real_time_y1-R2.real_time_y1 ); 
        else
            return   abs(R1.y1 - R2.y1); 
    }

    float get_distance_between_Rectangle(const Rectangle&R1, const Rectangle& R2, bool real_time = true)
    {
        if(real_time)
            return  0.5 * sqrt(  (R1.get_centre_rt_x()-R2.get_centre_rt_x())* (R1.get_centre_rt_x()-R2.get_centre_rt_x()) +  (R1.get_centre_rt_y()-R2.get_centre_rt_y()) *(R1.get_centre_rt_y()-R2.get_centre_rt_y()) );
        else
            return  0.5 * sqrt(  (R1.get_centre_x()-R2.get_centre_x())* (R1.get_centre_x()-R2.get_centre_x()) +  (R1.get_centre_y()-R2.get_centre_y()) *(R1.get_centre_y()-R2.get_centre_y())) ;
    }

    int calculate_distance(Rectangle rect1, Rectangle rect2) 
    {
        int distance = sqrt(( rect1.x1 + rect1.x2 - rect2.x1 - rect2.x2 )*( rect1.x1 + rect1.x2 - rect2.x1 - rect2.x2 )) *0.5 ;
        //   int distance = std::min(std::abs(rect1.x2 - rect2.x1), std::abs(rect2.x2 - rect1.x1));
        return distance;
    }

    int calculate_distance_adjacent_edge(Rectangle rect1, Rectangle rect2) 
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

 

       
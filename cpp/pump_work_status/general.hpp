#pragma once

#include<array>

#include <abi/consumer.hpp>

    constexpr std::array<int, 3> LAMP_LOWER_COLOR = {0, 0, 221};
    constexpr std::array<int, 3> LAMP_HIGHER_COLOR = {180, 30, 255};

    constexpr int LAMP_MIN_AREA_OF_BIG_ROOM = 35 * 35;
    constexpr int LAMP_MAX_AREA_OF_BIG_ROOM = 150 * 150;
    constexpr int LAMP_MIN_AREA_OF_SMALL_ROOM = 60 * 60;
    constexpr int LAMP_MAX_AREA_OF_SMALL_ROOM = 160 * 160;

    constexpr int LAMP_RECT_MIN_AREA_OF_BIG_ROOM = 50 * 50;
    constexpr int LAMP_RECT_MAX_AREA_OF_BIG_ROOM = 160 * 160;
    constexpr int LAMP_RECT_MIN_AREA_OF_SMALL_ROOM = 60 * 60;
    constexpr int LAMP_RECT_MAX_AREA_OF_SMALL_ROOM = 180 * 180;


    constexpr int LAMP_MAX_W_H_RATIO_OF_BIG_ROOM = 4;
    constexpr int LAMP_MAX_W_H_RATIO_OF_SMALL_ROOM = 3;

    constexpr int LAMP_MIN_LIGHT_BOX_OF_BIG_ROOM = 5; //NEW
    constexpr int LAMP_MIN_LIGHT_BOX_OF_SMALL_ROOM = 3; //NEW

    constexpr double BASE_PLATE_ROTATE_ANGLE_RATE = 1.5;
    constexpr int BASE_PLATE_DETECT_HEIGHT = 100;
    constexpr double FLOOR_AREA_RATE = 0.5;

    constexpr int MIN_WORK_EQUIPMENT_AREA = 10 * 10;
    constexpr int MAX_WORK_EQUIPMENT_AREA = 1000 * 1000;
    constexpr int WORK_EQUIPMENT_MIN_NUMBER = 2;

    constexpr double DETECT_THRESHOLD = 0.3;
    constexpr float PI = 3.141592653589793 ;
    constexpr int MIN_DETECT_SIZE = 30;
        
        static cv::Mat get_mask(cv::Mat& image, cv::Mat& mask_array, cv::Scalar fill_color=cv::Scalar(255, 255, 255), bool inverse=false) 
        {
            cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC3);

            cv::fillPoly(mask, mask_array, cv::Scalar(255, 255, 255));
            if (inverse) 
                cv::bitwise_not(mask, mask);

            return mask;
        }



        static std::vector<int> convert_xywh_to_xyxy(int x,int y,int w,int h)
        {
            std::vector<int> xyxy{0,0,0,0};
            xyxy[0] = x;
            xyxy[1] = y;
            xyxy[2] = x+w;
            xyxy[3] = y+h;
            return xyxy;
        }

        static std::vector<int> convert_xywh_to_xyxy(std::vector<int> xywh)
        {
            std::vector<int> xyxy{0,0,0,0};
            xyxy[0] = xywh[0];
            xyxy[1] = xywh[1];
            xyxy[2] = xywh[0]+xywh[2];
            xyxy[3] = xywh[1]+xywh[3];
            return xyxy;
        }

        static std::vector<int> convert_xyxy_to_xywh(int x1,int y1,int x2,int y2)
        {
            std::vector<int> xywh{0,0,0,0};
            xywh[0] = x1;
            xywh[1] = y1;  
            xywh[2] = x2-x1;
            xywh[3] = y2-y1;
            return xywh;
        }

         static std::vector<std::vector<int>> find_box_work(cv::Mat image, int min_area=0, int max_area=0) 
         {
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(image.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            std::vector<std::vector<int>> xyxy_list;

            int w = image.cols;
            int h = image.rows;

            if( min_area ||min_area)
                if(max_area==0 )
                    max_area = w*h;

            for (const auto& cnt : contours) 
            {
                auto area = cv::contourArea( cnt);
                
                if ((min_area <= area && area <= max_area) ) 
                {   
                    cv::Rect rect = cv::boundingRect(cnt);
                    std::vector<int> xywh = {rect.x, rect.y, rect.width, rect.height};
                    xyxy_list.push_back(convert_xywh_to_xyxy(xywh));
                }
            }

            return xyxy_list;
        }


        std::vector<std::vector<int>> find_box(cv::Mat image, int min_area=0, int max_area=0, int min_rect_area=0, int max_rect_area=0,
            double min_w_h_ratio=-0.5f, double max_w_h_ratio=-0.5f, std::string class_name="place") {
            std::vector<std::vector<cv::Point>> contours;
            std::vector<std::vector<cv::Point>> contours_copy;
            cv::findContours(image.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            std::vector<std::vector<int>> xyxy_list;

            int w = image.cols;
            int h = image.rows;

            if( min_area ||min_area)
            {
                if(max_area==0 )
                    max_area = w*h;
                for(auto& cnt : contours)
                {
                    auto area = cv::contourArea(cnt);
                    if(area  <max_area && area>min_area  )
                        contours_copy.push_back(cnt);
                }
                contours = contours_copy;
            }

            if (min_rect_area || max_rect_area )
            {
                if(!max_rect_area)
                    max_rect_area = w*h; 
            }

            if (min_w_h_ratio>0.f || max_w_h_ratio>0.f)
            {
                 if (min_w_h_ratio<0.f )
                    min_w_h_ratio=0.f;
                 if (max_w_h_ratio<0.f )
                    min_w_h_ratio=std::max(w, h);
            }

            if( !(min_area||max_area||min_rect_area||max_rect_area) &&min_w_h_ratio <0.f && max_w_h_ratio<0.f )
            {
                for (const auto& cnt : contours) 
                {
                    cv::Rect rect = cv::boundingRect(cnt);
                    std::vector<int> xywh = {rect.x, rect.y, rect.width, rect.height};
                    xyxy_list.push_back(convert_xywh_to_xyxy(xywh));
                }
                return xyxy_list;
            }

            for (const auto& cnt : contours) {
                cv::Rect rect = cv::boundingRect(cnt);
                int area = rect.width * rect.height;
                double rect_area = rect.width * rect.height;
                double w_h_ratio = std::max(rect.width, rect.height) / std::min(rect.width, rect.height);

                if ((min_area <= area && area <= max_area) &&
                    (min_rect_area <= rect_area && rect_area <= max_rect_area) &&
                    (min_w_h_ratio <= w_h_ratio && w_h_ratio <= max_w_h_ratio)) 
                    {
                        std::vector<int> xywh = {rect.x, rect.y, rect.width, rect.height};
                        xyxy_list.push_back(convert_xywh_to_xyxy(xywh));
                    }
            }

            return xyxy_list;
        }

        static  bool classify_lamp_status(cv::Mat& image, bool big_paint_room, std::vector<std::vector<int>>& mask_array) 
        {
            std::vector<std::vector<int>> new_mask_array(mask_array.size()+2);
            for(int i =0; i < mask_array.size(); i++)
                new_mask_array[i==0?i:i+2] = mask_array[i];

            new_mask_array[1]=std::vector<int> {mask_array[0][0],0};
            new_mask_array[2]=std::vector<int> {mask_array[1][0],0};

            if( !big_paint_room)
                new_mask_array[new_mask_array.size()-1][0]=0;

            int rows = new_mask_array.size();
            int cols = new_mask_array[0].size();

            cv::Mat new_mask_array_mask(rows, cols, CV_32SC1);
            for (int i = 0; i < rows; ++i) 
                for (int j = 0; j < cols; ++j) 
                    new_mask_array_mask.at<int>(i, j) = new_mask_array[i][j];
            
            cv::Mat mask = get_mask(image, new_mask_array_mask,cv::Scalar(255,255,255), true) ;
            cv::Mat masked_image = image & mask;
            cv::Mat erode_mask;
            cv::Mat erode_masked_image;
            cv::erode(mask, erode_mask, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(150, 150)));
            erode_masked_image = erode_mask&image;

            // 对图像进行处理
            cv::Mat hsv;
            cv::cvtColor(erode_masked_image, hsv, cv::COLOR_BGR2HSV);
            // 对光源进行颜色识别
            cv::Mat light_mask;
            cv::inRange(hsv, LAMP_LOWER_COLOR, LAMP_HIGHER_COLOR, light_mask);

            // 进行形态学操作
            cv::Mat open_light_mask, close_light_mask;
            cv::morphologyEx(light_mask, open_light_mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT,big_paint_room ? cv::Size(11, 11): cv::Size(25, 25)));

            cv::morphologyEx(open_light_mask, close_light_mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(30, 30)));

            // 查找光源的区域
            std::vector<std::vector<int>> light_box_list;
            if( big_paint_room)
                light_box_list = find_box(open_light_mask, LAMP_MIN_AREA_OF_BIG_ROOM, LAMP_MAX_AREA_OF_BIG_ROOM, LAMP_RECT_MIN_AREA_OF_BIG_ROOM, LAMP_RECT_MAX_AREA_OF_BIG_ROOM, 0, LAMP_MAX_W_H_RATIO_OF_BIG_ROOM);
            else
                light_box_list = find_box(open_light_mask, LAMP_MIN_AREA_OF_SMALL_ROOM, LAMP_MAX_AREA_OF_SMALL_ROOM, LAMP_RECT_MIN_AREA_OF_SMALL_ROOM, LAMP_RECT_MAX_AREA_OF_SMALL_ROOM, 0, LAMP_MAX_W_H_RATIO_OF_SMALL_ROOM);

            bool lamp_status = light_box_list.size() >= (big_paint_room?LAMP_MIN_LIGHT_BOX_OF_BIG_ROOM:LAMP_MIN_LIGHT_BOX_OF_SMALL_ROOM);
            return lamp_status;
        }

        static bool classify_base_plate_status(cv::Mat& image, bool big_paint_room,  std::vector<std::vector<int>>& mask_array) 
        {
            double angle = std::atan2(mask_array[mask_array.size()-1][0] - mask_array[0][0],
                                mask_array[mask_array.size()-1][1] - mask_array[0][1])
                                / PI * 180 * BASE_PLATE_ROTATE_ANGLE_RATE;
            int image_h = image.rows, image_w = image.cols;
            cv::Point2f center(image_w / 2.0, image_h / 2.0);
            cv::Mat M = cv::getRotationMatrix2D(center, -1 * angle, 1);

            cv::Mat rotated_img;
            cv::warpAffine(image, rotated_img, M, cv::Size(image_w, image_h));

            cv::Mat new_mask_array_mask(4, 2, CV_32SC1);
            for (int i = 0; i < 4; ++i) 
                for (int j = 0; j < 2; ++j) 
                    new_mask_array_mask.at<int>(i, j) = mask_array[i][j];

            cv::Mat mask = get_mask(rotated_img, new_mask_array_mask);

            cv::Mat rotated_mask;
            cv::warpAffine(mask, rotated_mask, M, cv::Size(image_w, image_h));

            cv::Mat gray_image;
            cv::cvtColor(rotated_mask, gray_image, cv::COLOR_BGR2GRAY);

            std::vector<int> mask_box = find_box(gray_image)[0];
            int x, y, w, h;
            auto xywh = convert_xyxy_to_xywh(mask_box[0], mask_box[1], mask_box[2], mask_box[3]);
            x = xywh[0];
            y = xywh[1];
            w = xywh[2];
            h = xywh[3];
            int x1 = x + w / 4;
            int y1 = y + h - BASE_PLATE_DETECT_HEIGHT;
            int x2 = x + 3 * w / 4;
            int y2 = y + h;
            x1 = std::max(0, x1);
            y1 = std::max(0, y1);
            x2 = std::min(image_w - 1, x2);
            y2 = std::min(image_h - 1, y2);

            cv::Mat cut_image = rotated_img(cv::Range(y1, y2), cv::Range(x1,x2));
            cv::Mat cut_mask =  rotated_mask(cv::Range(y1, y2), cv::Range(x1,x2));

            cut_image = cut_image & cut_mask;
            int cut_image_h = cut_image.rows;
            int cut_image_w = cut_image.cols;

            cv::Mat gray;
            cv::cvtColor(cut_image, gray, cv::COLOR_BGRA2GRAY);

            cv::Mat threshold;
            cv::adaptiveThreshold(gray, threshold, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 71, 3);

            cv::Mat black_hat;
            cv::morphologyEx(threshold, black_hat, cv::MORPH_BLACKHAT, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 80)));


            cv::Mat black_hat_gray;
            cv::cvtColor(cut_mask, black_hat_gray, cv::COLOR_BGR2GRAY);

            cv::Mat black_hat_tmp;
            cv::morphologyEx(~black_hat_gray, black_hat_tmp, cv::MORPH_DILATE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
            
            black_hat =  black_hat + black_hat_tmp;

            cv::Mat dilate;
            cv::morphologyEx(black_hat, dilate, cv::MORPH_DILATE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(10, 5)));

            cv::Mat open;
            cv::morphologyEx(dilate, open, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(cut_image_w / 4, cut_image_h * 0.9)));

            // std::cout<<"cv::countNonZero(open): "<<cv::countNonZero(open)<<std::endl;

            double floor_area_rate = cv::countNonZero(open) / static_cast<double>(cut_image_w * cut_image_h);
            bool has_base_plate = floor_area_rate <= FLOOR_AREA_RATE;

            // std::cout<<" has_base_plate:"<<has_base_plate<<std::endl;
            return has_base_plate;
        }

        static bool classify_work_equipment_status(cv::Mat& image, bool big_paint_room,  std::vector<std::vector<int> > &mask_array) 
        {
            cv::Mat new_mask_array_mask(mask_array.size(), 2, CV_32SC1);
            for (int i = 0; i < mask_array.size(); ++i) 
                for (int j = 0; j < 2; ++j) 
                    new_mask_array_mask.at<int>(i, j) = mask_array[i][j];

            cv::Rect bounding_rect = cv::boundingRect(new_mask_array_mask);

            int x2 = (bounding_rect.x + bounding_rect.width)<image.cols?(bounding_rect.x + bounding_rect.width) : image.cols-1   ;
            int y2 = (bounding_rect.y + bounding_rect.height)<image.rows?(bounding_rect.y + bounding_rect.height) : image.rows-1 ;

            cv::Mat cut_image = image(cv::Range(bounding_rect.y , y2), 
                                      cv::Range(bounding_rect.x , x2 ));

            cv::Mat cut_mask_array = new_mask_array_mask.clone();
            cut_mask_array.col(0) -= bounding_rect.x;
            cut_mask_array.col(1) -= bounding_rect.y;
            cv::Mat cut_mask = get_mask(cut_image, cut_mask_array);

            cv::bitwise_and(cut_image, cut_mask, cut_image);

            cv::Mat gray;
            cv::cvtColor(cut_image, gray, cv::COLOR_BGRA2GRAY);

            cv::Mat cut_mask_gray;
            cv::cvtColor(cut_mask, cut_mask_gray, cv::COLOR_BGRA2GRAY);

            cv::Mat blurred;
            cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0, 0);

            cv::Mat threshold;
            if (big_paint_room) {
                int block_size = 31;
                cv::adaptiveThreshold(blurred, threshold, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, block_size, 3);
            } else {
                cv::threshold(gray, threshold, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
            }
            threshold = ~threshold & cut_mask_gray;

            int hole_size = big_paint_room ? 15 : 20;

            cv::Mat erode_ada_threshold_h , erode_ada_threshold_v;
            cv::morphologyEx(threshold, erode_ada_threshold_h,  cv::MORPH_ERODE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(hole_size, 1)));
            cv::morphologyEx(threshold, erode_ada_threshold_v ,cv::MORPH_ERODE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, hole_size)));

            cv::Mat erode_ada_threshold = erode_ada_threshold_h + erode_ada_threshold_v;

            cv::Mat close_ada_threshold ;
            cv::morphologyEx(erode_ada_threshold, close_ada_threshold, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
            

            std::vector<std::vector<int >> work_equipment_contours;
            
            work_equipment_contours = find_box_work(
                close_ada_threshold,
                MIN_WORK_EQUIPMENT_AREA * (big_paint_room ? 1 : 3),
                MAX_WORK_EQUIPMENT_AREA
            );

            int work_equipment_number = work_equipment_contours.size();
            bool has_equipment_status = work_equipment_number >= WORK_EQUIPMENT_MIN_NUMBER;

            return has_equipment_status;
        }


        void clockwise_sort_by_left_corner( std::vector<std::vector<int>> &input_clockwise_sort )
        {
            float min_l2_zero_point = std::numeric_limits<float>::max();;  //xmin must be less than img cols
            int left_top_corner_index=0;
            for (size_t i = 0; i < input_clockwise_sort.size(); i++)
            {
                float l2_zero_point = sqrt((input_clockwise_sort[i][0]* input_clockwise_sort[i][0])+(input_clockwise_sort[i][1]* input_clockwise_sort[i][1])) ;
                if( l2_zero_point < min_l2_zero_point)
                {
                    min_l2_zero_point = l2_zero_point;
                    left_top_corner_index = i;
                }        
            }

            std::vector<std::vector<int>> output_clockwise_sort = input_clockwise_sort;
            for (size_t i = left_top_corner_index; i < (left_top_corner_index + input_clockwise_sort.size()); i++)
                output_clockwise_sort[i-left_top_corner_index] = input_clockwise_sort[ i%input_clockwise_sort.size() ];
            input_clockwise_sort = output_clockwise_sort;
            

        }
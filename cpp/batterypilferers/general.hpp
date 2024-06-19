#ifndef _GENERAL_HPP_
#define _GENERAL_HPP_

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"
#include <YoloFamily/Yolo_wrapper.hpp>

        using namespace glasssix;
        float MIN_BATTERY_CAR_SCALE=40;
        float AERO_RATE_BETWEEN_BATTERY_CAR_AND_PEOPLE=0.15;
        float IOU_BETWEEN_BATTERY_CAR=0.8;
        float IOU_BETWEEN_BATTERY=0.8;

        enum class order {
            NCHW,
            NHWC
        };

        struct Bbox
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
            float score;
            int frame_id;
            Bbox (int x11,int y11,int x22,int y22,int category_,float score_,int frame_index):x1(x11),x2(x22),y1(y11),y2(y22), category(category_), score(score_), frame_id(frame_index)
            {}
            Bbox (const Bbox& input):x1(input.x1),x2(input.x2),y1(input.y1),y2(input.y2), category(input.category), score(input.score), frame_id(input.frame_id)
            {}
            Bbox (int x11,int y11,int x22,int y22):x1(x11),x2(x22),y1(y11),y2(y22), category(-1), score(-1), frame_id(-1)
            {}

            Bbox& operator=(const Bbox& input)
            {
                if (this != &input) // 避免自我赋值
                {
                    x1 = input.x1;
                    x2 = input.x2;
                    y1 = input.y1;
                    y2 = input.y2;
                    category = input.category;
                    score = input.score;
                    frame_id = input.frame_id;
                }
                return *this;
            }
            
            int area()
            {
                return (y2-y1)*(x2-x1);
            }
        };
  
        struct car_person_batery
        {
            Bbox car;
            Bbox person;
            Bbox battery;

            car_person_batery(Bbox& car_,Bbox& person_,Bbox& battery_ ): car(car_),person(person_),battery(battery_)
            {}
        };

        float bbox_iou(Bbox &input1,Bbox &input2 )
        {
            float w = std::max(input1.x2, input2.x2 ) - std::min(input1.x1,input2.x1);
            float h = std::max(input1.y2,input2.y2 ) - std::min(input1.y1,input2.y1);
            float ww = input1.x2+input2.x2-input1.x1-input2.x1;
            float hh = input1.y2+input2.y2-input1.y1-input2.y1;
            if(ww<w||hh<h)
                return 0;
            else
                return  (ww-w)*(hh-h)/static_cast<float>( (input1.x2-input1.x1) *(input1.y2-input1.y1));
        }

        Bbox get_min_rect_in_car_person(Bbox& car,Bbox& person)
        {
            int x1_min = car.x1< person.x1?car.x1:person.x1;
            int y1_min = car.y1< person.y1?car.y1:person.y1;
            int x2_max = car.x2> person.x2?car.x2:person.x2;
            int y2_max = car.y2> person.y2?car.y2:person.y2;
            Bbox rect( x1_min,y1_min,x2_max,y2_max);
            return rect;
        }

        bool is_battery_in_rect(Bbox& rect, Bbox& battery)
        {
            bool in_rect = battery.x1>rect.x1 && battery.x2<rect.x2 && battery.y1>rect.y1 && battery.y2<rect.y2;
            return in_rect;
        }

      
        template <order data_order>
        static void concat_pic( std::vector<cv::Mat>& candicate_images, float *dst_data) 
        {
            std::array<float,3> means{123.675,116.28,103.53};
            std::array<float,3> stds{1.f/58.395,1.f/57.12,1.f/57.375};

            // std::array<float,3> means{0.0,0.0,0.0};
            // std::array<float,3> stds{1.f,1.f/57.12,1.f/57.375};

if constexpr(data_order==order::NHWC )
{
            int channels_size = candicate_images.size()*3;
            for (size_t i = 0; i < candicate_images.size(); i++)
            {
                uchar *ptr_sour = candicate_images[i].ptr<uchar>();
                for (size_t j = 0; j < candicate_images[i].rows*candicate_images[i].cols; j++)
                {                
                    dst_data[i*3+j*channels_size] = (ptr_sour[j*3] - means[0])*stds[0]; 
                     dst_data[i*3+j*channels_size+1] = (ptr_sour[j*3+1] - means[1])*stds[1]; 
                      dst_data[i*3+j*channels_size+2] = (ptr_sour[j*3+2] - means[2])*stds[2]; 
                }
            }
}
else
{
            int channels_size = candicate_images.size()*3;
            for (size_t i = 0; i < candicate_images.size(); i++)
            {
                uchar *ptr_sour = candicate_images[i].ptr<uchar>();
                for (size_t j = 0; j < candicate_images[i].rows*candicate_images[i].cols; j++)
                {      
                        dst_data[ i*3*candicate_images[i].rows*candicate_images[i].cols   + j ] = (ptr_sour[j*3] - means[0])*stds[0]; 
                        dst_data[ (i*3+1)*candicate_images[i].rows*candicate_images[i].cols   + j] = (ptr_sour[j*3+1] - means[1])*stds[1]; 
                        dst_data[ (i*3+2)*candicate_images[i].rows*candicate_images[i].cols   + j] = (ptr_sour[j*3+2] - means[2])*stds[2]; 
                }
            }
}

        }

        std::vector<Bbox> get_candicate_rect(std::vector<car_person_batery>& candicates1, 
                                    std::vector<car_person_batery>& candicates2 )
        {
            std::vector<Bbox> valid_crop_box_list;
            for(auto& candicate2 : candicates2)
                for(auto& candicate1 : candicates1)             
                    if(bbox_iou(candicate2.car,candicate1.car )>IOU_BETWEEN_BATTERY_CAR &&bbox_iou(candicate2.battery,candicate1.battery)<IOU_BETWEEN_BATTERY  )
                        valid_crop_box_list.push_back(get_min_rect_in_car_person(candicate2.car,candicate2.person));                
            return valid_crop_box_list;
        }


        std::vector<Bbox> get_candicate_rect(std::vector<car_person_batery>& candicates)
        {
            std::vector<Bbox> valid_crop_box_list;
            for(auto& candicate : candicates)
                    valid_crop_box_list.push_back(get_min_rect_in_car_person(candicate.car,candicate.person));                
            return valid_crop_box_list;
        }




        std::vector<car_person_batery> deal_one_frame(std::vector<Bbox>& input)
        {
            std::vector<car_person_batery> result;
            std::vector<Bbox> cars;
            std::vector<Bbox> batteries;
            std::vector<Bbox> persons;
            
            for(auto& var : input)
            {
                if(var.category==0)
                {
                    persons.push_back(var);
                }
                else if(var.category==1 && var.area()>MIN_BATTERY_CAR_SCALE )
                {
                    cars.push_back(var);
                }
                else if(var.category==2 )
                {
                    batteries.push_back(var);
                }
            }

            if(batteries.size()==0||cars.size()==0||persons.size()==0 )
            {
                return result;
            }
            else
            {
                for(auto& car : cars)
                {
                    for(auto& person : persons)
                    {
                        if( bbox_iou(person,car)>AERO_RATE_BETWEEN_BATTERY_CAR_AND_PEOPLE  )
                        {
                            Bbox rect = get_min_rect_in_car_person(car,person);
                            std::vector<Bbox> tmp_batteries;
                            for(auto& battery : batteries)
                            {
                                if( battery.area()<person.area()  &&  is_battery_in_rect(rect,battery))
                                {
                                    tmp_batteries.push_back(battery);
                                }
                            }
                            if(!tmp_batteries.size())
                            {
                                //当前两者无匹配电瓶
                            }
                            else
                            {
                                if(tmp_batteries.size()==1)
                                {
                                    result.emplace_back(car, person, tmp_batteries[0]);
                                }
                                else //获取置信度最大的电瓶
                                {
                                    int index = 0;
                                    float score = tmp_batteries[0].score;
                                    for (size_t i = 1; i < tmp_batteries.size(); i++)
                                    {
                                        if(tmp_batteries[i].score> score)
                                        {
                                            index = i;
                                            score = tmp_batteries[i].score;
                                        }
                                    }
                                    result.emplace_back(car, person, tmp_batteries[index]);

                                }
                            }
                        }
                    }
                }
            }
            //解析每帧数据
            //返回当前帧符合要求的车和人，电瓶数目
            return result;
        }

#endif // !_GENERAL_HPP_
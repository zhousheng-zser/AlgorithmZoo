      
#ifndef __WANDER_HPP__
#define __WANDER_HPP__
#include <map>
#include <cmath>

    struct return_info
    {
        int id;
        double first_show_time=0.f;
        double last_show_time=0.f;
        float  cosine_similarity=0.f;
        int detection_number = 1;
        int x1;
        int x2;
        int y1;
        int y2;
    };

    struct bbox
    {
        int id;
        int x1;
        int y1;
        int x2;
        int y2;

        void print()
        {
            std::cout<<x1<<" "<<x2<<" "<<y1<<" "<<y2<<"\n";
        }
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

        bbox get_bbox()
        {
            bbox box;
            box.x1 = x1;
            box.x2 = x2;
            box.y1 = y1;
            box.y2 = y2;
            return box;
        }

        bbox feature_fetch_regionof_body()
        {       
                bbox output;
                int centre_x = (x1+x2)/2;
                int centre_y = (y1+y2)/2;
                int width  = abs(x2- x1)*1.0;
                int height = abs(y2- y1)*1.0;
                output.x1 = centre_x - width/2;
                output.x2 = centre_x + width/2;
                output.y1 = centre_y - height/2;
                output.y2 = centre_y + height/2;
                return output;
        }


    };


  


    struct wander_info
    {
        std::array<float,2048> feature;            
        float   feature_sqrt_xx;
        double first_init_time;
        double last_match_time;
        bbox track_location;
        int detection_number; //  统计这个人 被检测到的次数
        void refresh_feature(const float *data)
        {
            std::memcpy(feature.data(), data, 2048*sizeof(float));
            float feature_sqrt_xx_temp = 0.f;
            for (size_t i = 0; i < 2048; i++)
            {
                feature_sqrt_xx_temp += (feature[i]*feature[i]);
            }       
            feature_sqrt_xx = sqrt(feature_sqrt_xx_temp);
         
        }

        void refresh_location(bbox& current_location)
        {
            track_location = current_location;
        }


    };


    float calculate_offset(bbox last_loca,bbox curr_loca) 
    {        
     
        int last_centre_x = (last_loca.x1 + last_loca.x2)/2  ;
        int last_centre_y = (last_loca.y1 + last_loca.y2)/2  ;
        int curr_centre_x = (curr_loca.x1 + curr_loca.x2)/2  ;
        int curr_centre_y = (curr_loca.y1 + curr_loca.y2)/2  ;
        float current_offset = sqrt(pow(curr_centre_x - last_centre_x , 2) + pow(curr_centre_y - last_centre_y , 2));

        // if(curr_loca.x1==1331&& curr_loca.x2==1392 && curr_loca.y1==476 && curr_loca.y2==632)
        // {
        //        last_loca.print();
        // }

        return current_offset;
    }


    int get_id(std::map<int, wander_info> & feature_table,int num)//get a new allocate id
    {
        int id=num-1;
        bool full=true;
        std::vector<int> mask(num);
        for (size_t i=0;i< num; i++)//对输出数据做处理
        {
            mask[i]=1;
        }
        
        std::map<int, wander_info> ::iterator it;
        for(it=feature_table.begin(); it != feature_table.end();  it++ )
        {   
            mask[it->first]=0;
        }

        for (size_t i=0; i< num; i++)//对输出数据做处理
        {
            if(mask[i]==1)
            {
                full=false;
                id = i;
                return id;
            }
        }
        return id;
    }


        static float cosine_similiar(const float *data1,const float* data2, float sqrt_xx,float sqrt_yy,int num=2048)
        {
            float xy=0.f;
            float xx=0.f;
            float yy=0.f;
            for(int i=0;i<num;i++)
            {
                xy += data1[i]*data2[i];
            }
            if(sqrt_xx * sqrt_yy<1e-7)
            {
                return -1;
            }
            return xy/(sqrt_xx*sqrt_yy); 
        }


        return_info feature_match(float *data,float sqrt_xx, double current_time, int devices,std::map<int, std::map<int, wander_info>>&  feature_tables, bbox curr_bbox , std::map<int,int>& allocated_id_current_frame, int table_size=200, float threshold=0.92)
        { 
            return_info person_result;
            std::map<int, wander_info> feature_table;
            if(feature_tables.count(devices))
            {   
                feature_table = feature_tables[devices];
            } 

            float currend_box_width = (curr_bbox.x2 - curr_bbox.x1)*1.5;
            float min_distance= 1000000000.f;

            int id_distance =-1;
            for (auto last_box: feature_table)
            {
                
                float tmp_min_distance = calculate_offset(last_box.second.track_location,curr_bbox );          
        
                if( min_distance > tmp_min_distance )
                {
                    min_distance = tmp_min_distance;
                    id_distance = last_box.first;
                }
            }
            
            std::map<int, wander_info> ::iterator it;
            float similiar =0.f;
            int id=-1;
            for(it=feature_table.begin(); it != feature_table.end();  it++ )
            {   
                float *data2 = feature_table[it->first].feature.data();
                auto tmp_similiar = cosine_similiar(data, data2, sqrt_xx, feature_table[it->first].feature_sqrt_xx );
                if(similiar < tmp_similiar )
                {
                    similiar = tmp_similiar;
                    id = it->first;
                }
            }

            if( min_distance <currend_box_width  && !allocated_id_current_frame.count(id_distance) )
            {
                

                float * data2 = feature_table[id_distance].feature.data();
                float cos_dis = cosine_similiar(data, data2, sqrt_xx, feature_table[id_distance].feature_sqrt_xx );
                if(cos_dis>0.85)
                {
                    allocated_id_current_frame[id_distance]=1;

                    feature_table[id_distance].last_match_time = current_time;
                    feature_table[id_distance].refresh_feature(data);
                    
                    feature_table[id_distance].refresh_location(curr_bbox);

                    person_result.first_show_time = feature_table[id_distance].first_init_time;
                    person_result.last_show_time  = current_time;
                    person_result.id = id_distance;
                    person_result.cosine_similarity = similiar;

                    feature_tables[devices]=feature_table;
                    return person_result;
                }

                //  feature_tables[devices]=feature_table;
            }


            if(similiar>threshold && !allocated_id_current_frame.count(id))
            {
                allocated_id_current_frame[id]=1;
                feature_table[id].last_match_time = current_time;
                feature_table[id].refresh_feature(data);
                feature_table[id].refresh_location(curr_bbox);
                person_result.first_show_time = feature_table[id].first_init_time;
                person_result.last_show_time  = current_time;
                person_result.id = id;
                person_result.cosine_similarity = similiar;
                feature_tables[devices]=feature_table;
                return person_result;
            }

      

            {
                auto id = get_id(feature_table, table_size);
                allocated_id_current_frame[id]=1;

                std::array<float,2048> feature;
                std::memcpy(feature.data(), data, 2048*sizeof(float));
                wander_info w_i;
                w_i.refresh_location(curr_bbox);
                w_i.feature = feature;
                w_i.feature_sqrt_xx = sqrt_xx;
                w_i.first_init_time = current_time;
                feature_table[id] = w_i;     
                person_result.id =id;

                person_result.first_show_time = current_time;
                person_result.last_show_time  = 0.f;
                person_result.cosine_similarity = 0.f;
                feature_tables[devices]=feature_table;

            }
            //  feature_tables[devices]=feature_table;
            return person_result;
        }


   



#endif
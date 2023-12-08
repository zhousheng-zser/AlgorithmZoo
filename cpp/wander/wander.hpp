      
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
    };

    struct wander_info
    {
        std::array<float,2048> feature;            
        float   feature_sqrt_xx;
        double first_init_time;
        double last_match_time;
    };


    float calculate_offset(bbox last_loca,bbox curr_loca) 
    {        
        int last_centre_x = (last_loca.x1 + last_loca.x2)/2  ;
        int last_centre_y = (last_loca.y1 + last_loca.y2)/2  ;
        int curr_centre_x = (curr_loca.x1 + curr_loca.x2)/2  ;
        int curr_centre_y = (curr_loca.y1 + curr_loca.y2)/2  ;

        // std::cout<< last_loca.x1<<" "<<last_loca.x2<<" "<<last_loca.y1<<" "<<last_loca.y2<<std::endl;
        // std::cout<< curr_loca.x1<<" "<<curr_loca.x2<<" "<<curr_loca.y1<<" "<<curr_loca.y2<<std::endl;
        // float width = curr_loca.x2 - curr_loca.x1;
        float current_offset = sqrt(pow(curr_centre_x - last_centre_x , 2) + pow(curr_centre_y - last_centre_y , 2));
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

        return_info feature_match(float *data,float sqrt_xx, double current_time, int devices,std::map<int, std::map<int, wander_info>>&  feature_tables, std::vector<bbox>& last_loca, bbox& curr_bbox , int table_size=200, float threshold=0.92)
        { 
            return_info person_result;
            std::map<int, wander_info> feature_table;
            if(feature_tables.count(devices))
            {   
                feature_table = feature_tables[devices];
            } 

            float currend_box_width = curr_bbox.x2 - curr_bbox.x1;
            float min_distance= 1000000000.f;

            int id_distance =-1;
            // std::cout<<"get last box: "<<last_loca.size()<<std::endl;
            for (auto last_box: last_loca)
            {
                float tmp_min_distance = calculate_offset(last_box,curr_bbox );
                // std::cout<<"tmp_min_distance match: "<<  tmp_min_distance <<std::endl;
                if( min_distance > tmp_min_distance )
                {
                    min_distance = tmp_min_distance;
                    id_distance = last_box.id;

                    //

                    
                    // std::cout<<"location match: "<<  id_distance <<std::endl;
                }
            }
            
            std::map<int, wander_info> ::iterator it;
            float similiar =0.f;
            int id=-1;
            for(it=feature_table.begin(); it != feature_table.end();  it++ )
            {   
                float *data2 = feature_table[it->first].feature.data();
                auto tmp_similiar = cosine_similiar(data, data2, sqrt_xx,feature_table[it->first].feature[2048] );
                if(similiar < tmp_similiar )
                {
                    // std::cout<<"tmp_similiar: "<<tmp_similiar<<std::endl;
                    similiar = tmp_similiar;
                    id = it->first;
                }
            }

            if( min_distance <currend_box_width )
            {
                feature_table[id_distance].last_match_time = current_time;
                person_result.first_show_time = feature_table[id_distance].first_init_time;
                person_result.last_show_time  = current_time;
//获取特征值并添加
                person_result.id = id_distance;
                person_result.cosine_similarity = similiar;
                // std::cout<<"location \n"<<id_distance<<std::endl;
                return person_result;
            }

            if(similiar>threshold)
            {
                // std::cout<<"threshold: "<<threshold<<std::endl;
                feature_table[id].last_match_time = current_time;
                person_result.first_show_time = feature_table[id].first_init_time;
                person_result.last_show_time  = current_time;
//获取特征值并添加
                person_result.id = id;
                person_result.cosine_similarity = similiar;
                // std::cout<<"simi\n"<<id<<std::endl;
                return person_result;
            }

      

            {
                auto id = get_id(feature_table, table_size);
                std::array<float,2048> feature;
                std::memcpy(feature.data(), data, 2048*sizeof(float));
                wander_info w_i;
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
            return person_result;
        }


   



#endif
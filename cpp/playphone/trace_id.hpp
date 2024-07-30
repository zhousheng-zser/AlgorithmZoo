#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <cmath>
// #include <unordered_map>
typedef struct
{
    int x1;
    int x2;
    int y1;
    int y2;
    float body_width;
    int play_num;
    int distance;
    int id;
    bool is_playphone;
    long long f;
    long long pf;
} Boxes_list;
static long long frame;
// 追踪
std::map<int32_t, Boxes_list> trace_id(
    std::map<int32_t, Boxes_list> trace_dic,
    const std::vector<Boxes_list>& boxes_list,
    long long f_now) {
    
    std::map<int32_t, Boxes_list> show_dic; // 用于此帧结果可视化的字典
    long long f = f_now;
    std::map<int32_t, Boxes_list> new_trace_dic = trace_dic; // 新的追踪字典

    if (!boxes_list.empty()) { // 如果此帧存在检测结果
        std::vector<std::vector<Boxes_list>> diff_all_list; // 所有检测结果的对比结果列表

        for (size_t i = 0; i < boxes_list.size(); ++i) {
            auto& box = boxes_list[i];
            int x1, x2, y1, y2;
            x1 = box.x1;
            x2 = box.x2;
            y1 = box.y1;
            y2 = box.y2;
            if (trace_dic.empty()) { // 如果历史结果为空，所有此帧结果新建
                Boxes_list box_list{ x1,x2,y1,y2,0,0,0,0,0,f,0 };
                trace_dic[i] = box_list;
                show_dic[i] = box_list;
            } else {
                // 比对
                std::vector<Boxes_list> diff_list;
                for (int i = 0; i < trace_dic.size(); i++)
                {
                    int x1_, x2_, y1_, y2_;
                    x1_ = trace_dic[i].x1;
                    x2_ = trace_dic[i].x2;
                    y1_ = trace_dic[i].y1;
                    y2_ = trace_dic[i].y2;
                    int new_box_center_x = (x1 + x2) / 2;
                    int new_box_center_y = (y1 + y2) / 2;
                    int his_box_center_x = (x1_ + x2_) / 2;
                    int his_box_center_y = (y1_ + y2_) / 2;
                    int distance = std::sqrt((new_box_center_y - his_box_center_y) * (new_box_center_y - his_box_center_y) + (new_box_center_x - his_box_center_x) * (new_box_center_x - his_box_center_x)); //求两个点的距离

                    if (distance < box.body_width * 0.1) {
                        Boxes_list diff_list_temp{ x1,x2,y1,y2,0,0,distance,i,0,f,0 };
                        diff_list.emplace_back( diff_list_temp);
                    }
                }
                 ////////////一般情况下,宽度不会为0,为0,表示没有数据 忽略
                if (diff_list.empty()) { // 如果此帧此结果与历史所有结果无联系，新建
                    int new_id = 1000;//todo
                    Boxes_list box_list{ x1,x2,y1,y2,0,0,0,0,0,f,0 };
                    trace_dic[new_id] = box_list;
                    show_dic[new_id] = box_list;
                }
                else {
                    std::sort(diff_list.begin(), diff_list.end(), [](const Boxes_list& a, const Boxes_list& b) {
                        return a.distance < b.distance;
                        });
                    diff_all_list.push_back(diff_list);
                }
            }

        }

        if (diff_all_list.empty()) { // 如果此帧所有结果没有与历史帧关联，此时返回结果
            return {trace_dic};//todo
        } else { // 若有关联，确保每个关联只被更新一次，只更新为最接近的关联
            std::map<int, Boxes_list> grouped_lists;

            for (const auto& boxdiflist : diff_all_list) {
                for (const auto& boxlist : boxdiflist)
                {
                    int key = boxlist.id;
                    if (!grouped_lists.count(key))
                    {
                        grouped_lists[key] = {};
                    }
                    std::vector<std::vector<Boxes_list>> list;
                    //list.push_back(boxlist);//todo todo
                    grouped_lists[key]=(boxlist);
                }
            }

            for (const auto& id_res : grouped_lists) {
                int new_x1 = id_res.second.x1;
                int new_x2 = id_res.second.x2;
                int new_y1 = id_res.second.y1;
                int new_y2 = id_res.second.y2;
                bool is_playing_phone = id_res.second.is_playphone;
                long long pf;
                int playing_num;
                if (is_playing_phone) {
                    playing_num = trace_dic[id_res.second.id].play_num + 1;
                    pf = f;
                }
                else
                {
                    playing_num = trace_dic[id_res.second.id].play_num;
                    pf = trace_dic[id_res.second.id].pf;
                }
                Boxes_list box_temp{ new_x1,new_x2,new_y1,new_y2,0,playing_num,0,0,0,f,pf };
                trace_dic[id_res.second.id] = box_temp;
            }
        }
    }

    if (!trace_dic.empty()) { // 存在追踪结果的情况下
        for (auto it: trace_dic) {
            if(f - it.second.f >= 125) // 5s内再未更新框体（消失于画面或者位移过大），删除此id
            { 
                new_trace_dic.erase(it.second.id);
            }
            if (f - it.second.pf >= 125) // 10s内更新都没检测为玩手机，玩手机计数置0
                new_trace_dic[it.second.id].play_num = 0;
            if (f > 9223372036854775807) // 重置帧上限
            {
                int new_x1 = it.second.x1;
                int new_x2 = it.second.x2;
                int new_y1 = it.second.y1;
                int new_y2 = it.second.y2;
                int new_playnum = it.second.play_num;
                int pf = it.second.pf;
                Boxes_list box_temp{ new_x1,new_x2,new_y1,new_y2,0,new_playnum,0,0,0,0,pf };
                new_trace_dic[it.second.id] = box_temp;
            }
        }
    }
    else
        return trace_dic;
    return { new_trace_dic};//todo
}

#include <iostream>

#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <mutex>
#include "wander.hpp"
#include <GenPipeline/GenPipeline.hpp>

namespace glasssix::wander
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)      
        net_feature_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/people_feature.rknn", device);   
        net_pedestrian_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_tumble_pedestrian_temp.rknn", device);// 后面会更新模型
#elif defined(USE_BMNN)
        net_feature_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/people_feature.bmodel", device);   
        net_pedestrian_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_tumble_pedestrian_temp.bmodel", device);// 后面会更新模型
        net_feature_->manual_possible_normalization(std::array<float, 3>{0.f, 0.f, 0.f}, std::array<float, 3>{1.0, 1.0, 1.0});
#endif 
        }

        void  Softmax(float* data, int num)
        {
            double L2_Sum = 0.f;
            for (size_t i = 0; i < num; i++)
            {
                data[i] = (exp(data[i]));
                L2_Sum += data[i];
            }
            for (size_t i = 0; i < num; i++)
            {
                data[i] = data[i] / L2_Sum;
            }
        }
        exposing::param_vector<wander::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map,const std::vector<PedestrianInfo> &pedestrain_info)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
                 
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                  throw exposing::abi_invalid_argument("incorrect roi in wander");

            std::vector<wander::box_info_internal> cate_result = run_detect(bitmap,height,width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);

            auto results = exposing::make_param_vector<wander::box_info>();

            for(auto& it:cate_result) 
            {
                    it.x1+=roi_x;
                    it.x2+=roi_x;
                    it.y1+=roi_y;
                    it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }


        std::string version()
		{
			const std::string algo_module_version = "2.1.0";
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", "", algo_module_version);
		}

        std::string remove_library(int devices)
        {
            delete_feature_library_by_id(devices);
            const std::string delete_library = "ok";
            return delete_library;
        }

        std::string remove_person_by_index(int devices,int id)
        {
            const std::string delete_library="OK";
            if(feature_tables.count(devices))
            {   
               delete_feature_library_person_in_one_library( feature_tables[devices], id) ;
            } 
            return delete_library;
        }

    private:

        bool delete_feature_library_person_in_one_library(std::map<int, wander_info>& feature_table, int id)
        {
            std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
            if(feature_table.count(id))
            {
                feature_table.erase(id);
                return true;
            }
            return false;

        }

        void delete_feature_library_by_id(int devices )
        {
            std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
            if(feature_tables.count(devices))
            {
                feature_tables.erase(devices);
            }
        }

        std::vector<box_info_internal> run_detect(const exposing::param_span<std::uint8_t>& bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, 
            std::map<std::string, double>& param_map, const std::vector<PedestrianInfo> &pedestrain_info)
        {
            double conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.7f;
            double device_id               = param_map.count("device_id") ? param_map["device_id"] : 0;
            double feature_table_size      = param_map.count("feature_table_size") ? param_map["feature_table_size"] : 10000.f;      
            double current_time            = param_map.count("current_time") ? param_map["current_time"] : 0.f;
            float feature_match_threshold  = param_map.count("feature_match_threshold") ? param_map["feature_match_threshold"] : 0.92f;

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

            std::vector<box_info_internal> l_c; 
            std::vector<bbox> temp_last_location_info;

            std::map<int,int> allocate_id_current_frame;
            for(auto& head:pedestrain_info)
            {
                safe_crop_rect person_bbox(head.x1,head.x2,head.y1,head.y2,width,height);
                auto body = person_bbox;
                // auto body = person_bbox.feature_fetch_regionof_body();

                bbox tmp_bbox;
                int x1=std::round( head.x1)>0?std::round( head.x1):0  ;
                int y1=std::round( head.y1)>0?std::round( head.y1):0  ;
                int x2=std::round( head.x2)<width ? std::round( head.x2):width ;
                int y2=std::round( head.y2)<height? std::round( head.y2):height ;

                tmp_bbox.x1 =  person_bbox.x1;
                tmp_bbox.x2 =  person_bbox.x2;
                tmp_bbox.y1 =  person_bbox.y1;
                tmp_bbox.y2 =  person_bbox.y2;

                cv::Mat crop = image(cv::Range( std::round(body.y1), std::round(body.y2) ), cv::Range( std::round(body.x1), std::round(body.x2))).clone();

                cv::Mat headimg, pedestrian;
                cv::resize(crop, pedestrian, cv::Size((int)(80), (int)(80)));
                cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);//检测是否为行人不能做这个操作
                cv::resize(crop, headimg, cv::Size((int)(128), (int)(256)), cv::INTER_CUBIC);

                auto  data_objects = net_pedestrian_->forward(pedestrian).begin()->second->cpu_data();
                int category = 4;
                Softmax(data_objects,category);
                int index = std::max_element(data_objects, data_objects + category) - data_objects;
                double confidence = data_objects[index];
                // 0，正常站立 1，攀爬 2，跌倒 3，不是人 ;且分数低于0.7的不检测
                if(confidence < conf_thres || index == 3)
                    continue;
                auto  data = net_feature_->forward(headimg).begin()->second->cpu_data();
                float xx = 0.f;

                for(int i=0; i<2048; i++)
                    xx += data[i] * data[i] ;

                auto sqrt_xx=sqrt(xx);
                std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
                std::vector<float> feature(data, data + 2048);
                auto person_info = feature_match(feature.data(), sqrt_xx,current_time, std::round(device_id), feature_tables, person_bbox.get_bbox(), allocate_id_current_frame, feature_table_size, feature_match_threshold );
             
                    box_info_internal result;
                    result.x1=person_bbox.x1 ;
                    result.y1=person_bbox.y1 ;
                    result.x2=person_bbox.x2 ;
                    result.y2=person_bbox.y2 ;                 
                    result.confidence = head.score ;

                    result.id = person_info.id;
                    result.first_show_time = person_info.first_show_time;
                    result.last_show_time = person_info.last_show_time;
                    result.cosine_similarity= person_info.cosine_similarity;
                    result.detection_number = person_info.detection_number;
                l_c.emplace_back(result);
                tmp_bbox.id =  person_info.id;
                temp_last_location_info.push_back(tmp_bbox);
            }

            last_location_info.clear();
            last_location_info = temp_last_location_info;


            return l_c;
        }


    private:

        std::shared_ptr<GenPipeline> net_feature_;
        std::shared_ptr<GenPipeline> net_pedestrian_;
        std::string model_directory_;
        int device_ ;

    public:
        static std::vector<bbox> last_location_info;
        static std::mutex Feature_Table_Mutex;
        static std::map<int, std::map<int, wander_info>>  feature_tables;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {

    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<wander::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map, const std::vector<PedestrianInfo>& pedestrain_info) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

    std::string detect_code_internal::remove_library(int id)
    {
        return impl_->remove_library(id);
    }

    std::string detect_code_internal::remove_person_by_index(int device_id,int id)
    {
        return impl_->remove_person_by_index(device_id,id);
    }

    std::map<int, std::map<int, wander_info>> detect_code_internal::impl::feature_tables;
    std::vector<bbox> detect_code_internal::impl::last_location_info;
    std::mutex detect_code_internal::impl::Feature_Table_Mutex;
}

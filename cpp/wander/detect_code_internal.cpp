#include <iostream>

#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>

#include "../pedestrian/classify_code.hpp"
#include "hardcode.hpp"
#include <mutex>
#include "general.hpp"
#include "wander.hpp"
namespace glasssix::wander
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
            :impl{get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device} 
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
            : net_feature_(phai, model_directory + std::string("/people_feature.rknn"), device), model_directory_(model_directory)
        {   
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("pedestrian");
            pedestrain_instance_ = glasssix::exposing::make_exported_interface<pedestrian::classify_code>(exposing::param_string(model_directory), device);
        }



        exposing::param_vector<wander::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map)
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
            {
                  throw exposing::abi_invalid_argument("incorrect roi in wander");
            }

            // cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

            std::vector<wander::box_info_internal> cate_result = run_detect(bitmap,height,width, roi_x, roi_y, roi_width, roi_height, param_map);

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
			const std::string algo_module_version = "1.1.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

            exposing::param_string nn_frame_version_param= pedestrain_instance_.version();
#else
            exposing::param_string nn_frame_version_param = pedestrain_instance_.version();
#endif
            std::string nn_frame_version =  exposing::to_narrow_string(nn_frame_version_param);
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
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

        std::vector<box_info_internal> run_detect(const exposing::param_span<std::uint8_t>& bitmap, int height,int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map)
        {
            double device_id               = param_map.count("device_id") ? param_map["device_id"] : 0;
            double feature_table_size      = param_map.count("feature_table_size") ? param_map["feature_table_size"] : 10000.f;      
            double current_time            = param_map.count("current_time") ? param_map["current_time"] : 0.f;
            float feature_match_threshold  = param_map.count("feature_match_threshold") ? param_map["feature_match_threshold"] : 0.92f;
            float conf_threshold           = param_map.count("person_conf") ? param_map["person_conf"] : 0.7f;
            float iou_threshold =  0.45f;   

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            empty_map_abi.add_or_update("conf_thres", conf_threshold);
            empty_map_abi.add_or_update("nms_thres", iou_threshold);
            empty_map_abi.add_or_update("wander", 1.f);

            exposing::param_vector<pedestrian::box_info> pedestrian_info_list = pedestrain_instance_.detect(bitmap, 3, height, width, roi_x, roi_y, roi_width, roi_height, empty_map_abi);
            std::vector<PostureInfo> pedestrain_info; 


            for (auto pinfo : pedestrian_info_list) 
            {
                PostureInfo postureInfo{ pinfo };
                pedestrain_info.push_back(postureInfo);
            }

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * 3 * height * width);

            std::vector<box_info_internal> l_c; 
            std::vector<bbox> temp_last_location_info;

            // std::cout<<"before:  "<<last_location_info.size()<<std::endl;
            // std::cout<< pedestrain_info.size()<<"pedestrain_info\n";

            std::map<int,int> allocate_id_current_frame;
            for(auto& head:pedestrain_info)
            {

                safe_crop_rect person_bbox(head.x1,head.x2,head.y1,head.y2,width,height);
                auto body = person_bbox.feature_fetch_regionof_body();
                bbox tmp_bbox;
                int x1=std::round( head.x1)>0?std::round( head.x1):0  ;
                int y1=std::round( head.y1)>0?std::round( head.y1):0  ;
                int x2=std::round( head.x2)<width ? std::round( head.x2):width ;
                int y2=std::round( head.y2)<height? std::round( head.y2):height ;

                tmp_bbox.x1 =  person_bbox.x1;
                tmp_bbox.x2 =  person_bbox.x2;
                tmp_bbox.y1 =  person_bbox.y1;
                tmp_bbox.y2 =  person_bbox.y2;

                cv::Mat crop = image(cv::Range( std::round(body.y1), std::round(body.y2) ), cv::Range( std::round(body.x1), std::round(body.x2)));

                cv::Mat headimg;
                // cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                cv::resize(crop, headimg, cv::Size((int)(128), (int)(256)), cv::INTER_CUBIC);
                cv::transpose(headimg, headimg);
                auto  network_result = net_feature_.forward(headimg.data, { 1, headimg.rows, headimg.cols,headimg.channels() }, RKNN_TENSOR_NHWC);

                float *data1=network_result["865"]->mutable_cpu_data();

                float xx = 0.f;
                for(int i=0; i<2048; i++)
                {
                    xx += data1[i] * data1[i] ;
                }
                auto sqrt_xx=sqrt(xx);
                std::lock_guard<std::mutex> lock(Feature_Table_Mutex);

                auto person_info = feature_match(data1, sqrt_xx,current_time, std::round(device_id), feature_tables, person_bbox.get_bbox(), allocate_id_current_frame, feature_table_size, feature_match_threshold );
             
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
                l_c.emplace_back(result);
                tmp_bbox.id =  person_info.id;
                temp_last_location_info.push_back(tmp_bbox);
            }

            last_location_info.clear();
            last_location_info = temp_last_location_info;


            return l_c;
        }


    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

	
        rknnwrapper::rknn_wrapper net_feature_;
#else

        std::unique_ptr<excalibur::pipeline<float>> net_feature_;
#endif
        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;
        std::string model_directory_;
        pedestrian::classify_code pedestrain_instance_;
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

    exposing::param_vector<wander::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
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

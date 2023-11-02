#include <iostream>

#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>

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
            :net_detect_(phai,  model_directory + std::string("/people_detect.rknn"), device), net_feature_(phai, model_directory + std::string("/people_feature.rknn"), device), model_directory_(model_directory)
        {   
            init_data(posture_add_weight, posture_mul_weight);
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

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

            std::vector<wander::box_info_internal> cate_result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

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
            const std::string algo_module_version = "1.0.2";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string nn_frame_version = net_detect_.version();
#else
            std::string nn_frame_version = net_detect_.version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

        std::string remove_library(int id)
        {
            delete_feature_library_one(id);
            const std::string delete_library = "ok";
            return fmt::format(R"({{"delete_library: ":"{}", "device_id: ":"{}" }})", delete_library, id);
        }

    private:
        void delete_feature_library_one(int devices )
        {
            std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
            if(feature_tables.count(devices))
            {
                feature_tables.erase(devices);
            }
        }

        std::vector<box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map)
        {
            double device_id               = param_map.count("device_id") ? param_map["device_id"] : 0;
            double feature_table_size      = param_map.count("feature_table_size") ? param_map["feature_table_size"] : 10000.f;      
            double current_time            = param_map.count("current_time") ? param_map["current_time"] : 0.f;
            float feature_match_threshold  = param_map.count("feature_match_threshold") ? param_map["feature_match_threshold"] : 0.92f;
            float conf_threshold           = param_map.count("person_conf") ? param_map["person_conf"] : 0.7f;
            float iou_threshold =  0.45f;   

            auto new_shape = cv::Size(640,  640);
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;

            auto  network_results = net_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
         
            std::vector<std::string>  out_names={"355","340","output0"};
            
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
        
            for (size_t i=0;i< out_names.size(); i++)//process the output
            {
                forwards.push_back(network_results[out_names[i]]);
            }
   
            auto real_output = Yovo8s_Concat(forwards,posture_add_weight ,posture_mul_weight);//5*8400

            auto nms_result = post_process(real_output,pad_h,pad_w, 1.f/ratio,conf_threshold);

            std::vector<box_info_internal> l_c; 
            for(auto& head:nms_result)
            {
                int x1=std::round( head[0])>0?std::round( head[0]):0  ;
                int y1=std::round( head[1])>0?std::round( head[1]):0  ;
                int x2=std::round( head[2])<image.cols ? std::round( head[2]):image.cols ;
                int y2=std::round( head[3])<image.rows ? std::round( head[3]):image.rows ;

                if( (y2-y1)<0 ||(x2-x1)<0 )
                {
                    continue;
                }

                cv::Mat crop = image(cv::Range(y1,y2), cv::Range(x1,x2));
                cv::Mat headimg;
                cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                cv::resize(crop, headimg, cv::Size((int)(128), (int)(256)), cv::INTER_CUBIC);
                auto  network_result = net_feature_.forward(headimg.data, { 1, headimg.rows, headimg.cols,headimg.channels() }, RKNN_TENSOR_NHWC);

                float *data1=network_result["865"]->mutable_cpu_data();

                float xx = 0.f;
                for(int i=0; i<2048; i++)
                {
                    xx += data1[i] * data1[i] ;
                }
                auto sqrt_xx=sqrt(xx);
                std::lock_guard<std::mutex> lock(Feature_Table_Mutex);
                auto person_info = feature_match(data1, sqrt_xx,current_time, std::round(device_id), feature_tables, feature_table_size, feature_match_threshold );

                box_info_internal result;
                    result.x1=x1>0?x1:0 ;
                    result.y1=y1>0?y1:0 ;
                    result.x2=x2<image.cols ?x2:image.cols ;
                    result.y2=y2<image.rows ?y2:image.rows ;                 
                    result.confidence = head[4] ;

                    result.id = person_info.id;
                    result.first_show_time = person_info.first_show_time;
                    result.last_show_time = person_info.last_show_time;
                    result.cosine_similarity= person_info.cosine_similarity;
                l_c.emplace_back(result);
            }
            return l_c;
        }


    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_detect_;
        rknnwrapper::rknn_wrapper net_feature_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_detect_;
        std::unique_ptr<excalibur::pipeline<float>> net_feature_;
#endif
        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;
        std::string model_directory_;
        
        int device_ ;

    public:
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

    std::map<int, std::map<int, wander_info>> detect_code_internal::impl::feature_tables;
    std::mutex detect_code_internal::impl::Feature_Table_Mutex;
}

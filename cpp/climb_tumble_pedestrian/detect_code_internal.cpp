#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

namespace glasssix::climb_tumble_pedestrian
{
    inline int compute_area(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
        int x = std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1));
        int y = std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
        return x * y;
    }

    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{exposing::to_narrow_string(model_directory), device}
        {
        }

        impl( std::string model_directory, int device)
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_climb_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_tumble_pedestrian.rknn", device);
#elif defined(USE_BMNN)
            net_climb_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/climbing_tumble_pedestrian.bmodel", device);
#else
            net_climb_ = std::make_shared<GenPipeline>(get_model_params("climb_20240426cut"), std::string(model_directory) + "/climb_20240426cut.racy", device);
#endif
            net_climb_->manual_possible_normalization(std::array<float,3>{104.f, 117.f, 123.f},std::array<float,3>{1.f/128.f, 1.f/128.f, 1.f/128.f});
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
        exposing::param_vector<climb_tumble_pedestrian::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,  std::map<std::string, float>& param_map,const std::vector<PedestrianInfo> &pedestrain_info)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in climb_tumble_pedestrian");

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

   //         auto climb_objects = yolov8_instance->get_objects( image, con_thres, nms_thres );
   //         auto climb_objects = net_climb_->forward(image).begin()->second;
   //         auto tensor_out_data = tensor_out->mutable_cpu_data();
			//int index = std::max_element(result, result + 1) - result;
            auto results = exposing::make_param_vector<climb_tumble_pedestrian::box_info>();
            std::vector<climb_tumble_pedestrian::box_info_internal> tumble_results;
            std::vector<climb_tumble_pedestrian::box_info_internal> boxs;
            for (auto pinfo : pedestrain_info) 
            {
                if(pinfo.x1 < 0 || pinfo.x2 > width || pinfo.y1 < 0 || pinfo.y2 > height)
                {
                    continue;
                }
                climb_tumble_pedestrian::box_info_internal box;
                box.x1 = pinfo.x1;
                box.y1 = pinfo.y1;
                box.x2 = pinfo.x2;
                box.y2 = pinfo.y2;
                //int x1,x2,y1,y2;
                //x1 = pinfo.x1;
                //x2 = pinfo.x2;
                //y1 = pinfo.y1;
                //y2 = pinfo.y2;
                cv::Mat body;
                cv::Mat crop = image(cv::Range(box.y1, box.y2), cv::Range(box.x1, box.x2)).clone();
                // cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                cv::resize(crop, body, cv::Size((int)(80), (int)80));
                auto climb_objects = net_climb_->forward(body).begin()->second->mutable_cpu_data();
                int category = 4;
                Softmax(climb_objects,category);
                int index = std::max_element(climb_objects, climb_objects + category) - climb_objects;
                box.confidence= climb_objects[index];
                if(box.confidence < con_thres)
                    continue;
                // 0，正常站立1，攀爬2，跌倒3，不是人
                box.category = index;
                if(index != 2)
                    results.push_back(glasssix::exposing::make_as_first<box_info_impl>(box));
                else 
                    tumble_results.emplace_back(box);
            }
            int device_id = std::round(param_map.count("device_id") ? param_map["device_id"] : 0.f);
            { // 控制list_tumble_mutex 生命周期
                std::lock_guard<std::mutex> lock(list_tumble_mutex);
                std::vector<climb_tumble_pedestrian::box_info_internal>& tumble_old = list_tumble_map[device_id];
                // 检测当前的
                for (auto tumble : tumble_results)
                {
                    float mx = 1e9;
                    climb_tumble_pedestrian::box_info_internal  mx_val;
                    for (auto old : tumble_old)
                    {
                        float x1 = (old.x1 + old.x2 >> 1), y1 = (old.y1 + old.y2 >> 1);
                        float x2 = (tumble.x1 + tumble.x2 >> 1), y2 = (tumble.y1 + tumble.y2 >> 1);
                        float len = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
                        if (len < mx)
                        {
                            mx_val = old;
                            mx = len;
                        }
                    }
                    if (mx > 5e8)
                        continue;
                    int area = compute_area(mx_val.x1, mx_val.y1, mx_val.x2, mx_val.y2, tumble.x1, tumble.y1, tumble.x2, tumble.y2);
                    if (area / ((mx_val.x2 - mx_val.x1) * (mx_val.y2 - mx_val.y1) + (tumble.x2 - tumble.x1) * (tumble.y2 - tumble.y1) - area) > 0.7)
                        results.push_back(glasssix::exposing::make_as_first<box_info_impl>(tumble));
                }
                //保存当前的  
                list_tumble_map[device_id].clear();
                list_tumble_map[device_id] = tumble_results;
            }
            return results;
        }

        std::string version()
		{
			const std::string algo_module_version = "2.0.0";
            std::string nn_frame_version = net_climb_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:
        std::string model_directory_;
        int device_;
        std::shared_ptr<GenPipeline> net_climb_;
        std::shared_ptr<Yolov8<GenPipeline, true, true>> yolov8_instance;
        static std::mutex list_tumble_mutex;
        static std::map<int, std::vector<climb_tumble_pedestrian::box_info_internal> >list_tumble_map;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<climb_tumble_pedestrian::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map, const std::vector<PedestrianInfo>& pedestrain_info) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

    std::mutex detect_code_internal::impl::list_tumble_mutex;
    std::map<int, std::vector<climb_tumble_pedestrian::box_info_internal> >detect_code_internal::impl::list_tumble_map;
}

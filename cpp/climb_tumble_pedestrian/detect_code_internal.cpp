#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include "general.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif

#include <Primitives/tensor_conversions.hpp>

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
            net_detect_light = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("climbing_tumble_pedestrian", false),
                std::string(model_directory) + "/" + "climbing_tumble_pedestrian.rknn", device);
#else
            net_detect_light = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("climbing_tumble_pedestrian", false),
                std::string(model_directory) + "/" + "climbing_tumble_pedestrian.racy", device);
#endif  
            init_data_compatible(256, 256, add_weight_light, mul_weight_light);
        }
        void init_data_compatible(int width, int height, std::vector<float>& add_weight, std::vector<float>& mul_weight)
        {
            int size_mul_weight = width * height * 21 / 1024; //33600
            int size_add_weight = 2 * size_mul_weight;
            int width_base = width / 8;
            int height_base = height / 8;
            int candicate_area = width_base * height_base; //160*160

            add_weight.resize(size_add_weight);
            mul_weight.resize(size_mul_weight);
            for (size_t i = 0; i < candicate_area * 21 / 16; i++)
            {
                if (i < candicate_area) // 25600
                {
                    add_weight[i] = i % (width_base); //160
                    add_weight[i + size_mul_weight] = i / (width_base); //
                    mul_weight[i] = 8.f;
                }
                else if (i<int(std::round(i - candicate_area * 1.25)))
                {
                    add_weight[i] = (i - candicate_area) % (width_base / 2);
                    add_weight[i + size_mul_weight] = (i - candicate_area) / (width_base / 2);
                    mul_weight[i] = 16.f;
                }
                else
                {
                    add_weight[i] = int(std::round(i - candicate_area * 1.25)) % (width_base / 4);
                    add_weight[i + size_mul_weight] = int(std::round(i - candicate_area * 1.25)) / (width_base / 4);
                    mul_weight[i] = 32.f;
                }
            }
            return;
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
            float con_thres_tumble = param_map.count("conf_thres_tumble") ? param_map["conf_thres_tumble"] : 0.7f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

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
                cv::Mat crop = image(cv::Range(box.y1, box.y2), cv::Range(box.x1, box.x2)).clone();

                std::vector<float> cropped_result = yolo8_detect(crop, 256, 256);// 分类 
                int category = 5;
                int index = std::max_element(cropped_result.begin(), cropped_result.begin() + category) - cropped_result.begin();

                box.confidence= cropped_result[index];
                if(box.confidence < con_thres)
                    continue;
                // 0，正常站立1，攀爬2，跌倒3，不是人
                box.category = index;
                if(index != 2)
                    results.push_back(glasssix::exposing::make_as_first<box_info_impl>(box));
                else if(box.confidence >= con_thres_tumble)    //跌倒置信度0.7
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
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", algo_module_version, algo_module_version);
		}
        std::vector<float> yolo8_detect(cv::Mat& image, int w_, int h_)
        {
            auto new_shape = cv::Size(w_, h_);
            cv::Mat blob;
            float ratio = 0;
            int pad_h = 0;
            int pad_w = 0;
            std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            std::shared_ptr<memory::tensor<float>> real_forwards;

            auto network_result = net_detect_light->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
            float* light_conf = network_result["output0"]->mutable_cpu_data();
            std::vector<float> current_frame_result;
            current_frame_result.push_back(light_conf[0]);
            current_frame_result.push_back(light_conf[1]);
            current_frame_result.push_back(light_conf[2]);
            current_frame_result.push_back(light_conf[3]);
            current_frame_result.push_back(light_conf[4]);
            return current_frame_result;

        }

    private:
        std::string model_directory_;
        int device_;
        std::vector<float> add_weight_light;
        std::vector<float> mul_weight_light;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_light;
#else
        std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_light;
#endif

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

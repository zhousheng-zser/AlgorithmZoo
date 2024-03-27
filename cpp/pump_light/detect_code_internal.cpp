#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
//#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include "general.hpp"


#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif

#include <Primitives/tensor_conversions.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>



namespace glasssix::pump_light
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device } 
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_detect_light = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("pump_light", false),
                std::string(model_directory) + "/" + "pump_light.rknn", device);

#else
            net_detect_light = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("pump_light", false),
                std::string(model_directory) + "/" + "pump_light.racy", device);
#endif  
            init_data_compatible(128, 128, add_weight_light, mul_weight_light);
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

        pump_light::box_info detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);


            pump_light::box_info result = run_detect(image, height, width, param_map);

            return std::move(result);
        }

        std::string version()
        {
            const std::string nn_frame_version = "2.0.0";

            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, "");
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
            return current_frame_result;

        }

    private:

        /**
           * @fun run_detect
           * @param image param_map
           * @details run detect (maybe in multithreading)
        */
        pump_light::box_info run_detect(cv::Mat& image, int height, int width, std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.8f;
            float x1 = param_map.count("x1") ? param_map["x1"] : 0.0f;
            float y1 = param_map.count("y1") ? param_map["y1"] : 0.0f;
            float x2 = param_map.count("x2") ? param_map["x2"] : 0.0f;
            float y2 = param_map.count("y2") ? param_map["y2"] : 0.0f;
            float x3 = param_map.count("x3") ? param_map["x3"] : 0.0f;
            float y3 = param_map.count("y3") ? param_map["y3"] : 0.0f;
            float x4 = param_map.count("x4") ? param_map["x4"] : 0.0f;
            float y4 = param_map.count("y4") ? param_map["y4"] : 0.0f;
            if (x1<0 || x1>width || y1 > height || y1 < 0) {
                throw exposing::abi_invalid_argument("incorrect param_map in pump_light x1<0 || x1>width || y1 > height || y1 < 0");
            }
            if (x2<0 || x2>width || y2 > height || y2 < 0) {
                throw exposing::abi_invalid_argument("incorrect param_map in pump_light x2<0 || x2>width || y2 > height || y2 < 0");
            }
            if (x3<0 || x3>width || y3 > height || y3 < 0) {
                throw exposing::abi_invalid_argument("incorrect param_map in pump_light x3<0 || x3>width || y3 > height || y3 < 0");
            }
            if (x4<0 || x4>width || y4 > height || y4 < 0) {
                throw exposing::abi_invalid_argument("incorrect param_map in pump_light x4<0 || x4>width || y4 > height || y4 < 0");
            }

            int mi_x, mi_y, mx_x, mx_y;
            mi_x = std::min(std::min(x1, x2), std::min(x3, x4));
            mi_y = std::min(std::min(y1, y2), std::min(y3, y4));
            mx_x = std::max(std::max(x1, x2), std::max(x3, x4));
            mx_y = std::max(std::max(y1, y2), std::max(y3, y4));

            cv::Mat cropped_image = image(cv::Range(mi_y, mx_y), cv::Range(mi_x, mx_x)).clone();
            std::vector<float> cropped_result = yolo8_detect(cropped_image, 128, 128);// 灯光检测 
            
            pump_light::box_info_internal ans;
            ans.light_status = (cropped_result[0]<= cropped_result[1] && con_thres <=cropped_result[1]) ?1:0;
            ans.score = cropped_result[ans.light_status];
            return exposing::make_as_first<box_info_impl>(ans);
        }
        std::string model_directory_;
        int device_;
        std::vector<float> add_weight_light;
        std::vector<float> mul_weight_light;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_light;  
#else
        std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_light;
#endif

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    pump_light::box_info detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, param_map);
    }
}

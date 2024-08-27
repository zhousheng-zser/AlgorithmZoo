#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
//#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>
// #include "general.hpp"

#include <GenPipeline/GenPipeline.hpp>


#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>



namespace glasssix::pump_light
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device,int model_type)
            : model_directory_{ std::string(model_directory) }, device_{ device }, model_type_{ model_type }
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string model_ext{ ".rknn" };
#elif defined(USE_BMNN)
            std::string model_ext{ ".bmodel" };
#else
            std::string model_ext{ ".onnx" };
#endif
            if(model_type == 0)
                net_detect_light = std::make_shared<GenPipeline>(model_directory_ + "/pump_light" + model_ext, device);
            else if(model_type == 1)
                net_detect_light = std::make_shared<GenPipeline>(model_directory_ + "/pump_light_32" + model_ext, device);
            net_detect_light->manual_possible_normalization(0, 1.f / 255);
        } 

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, int& pad_h, int& pad_w, cv::Size input_shape = cv::Size(640, 640))
        {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) / 2);
                pad_w = int((input_shape.width - cut_image.cols) / 2);
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return { mask_image,scale };
        }
        std::tuple<cv::Mat, float> preprocess_detection_32(cv::Mat& src, int& pad_h, int& pad_w, cv::Size input_shape = cv::Size(640, 640))
        {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat mask_image;
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, mask_image, input_shape, cv::INTER_LINEAR);
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return { mask_image,scale };
        }

        pump_light::box_info detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));


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
            if(model_type_ == 0)
                std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
            else if(model_type_ == 1)
                std::tie(blob, ratio) = preprocess_detection_32(image, pad_h, pad_w, new_shape);
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            std::shared_ptr<memory::tensor<float>> real_forwards;

            auto network_result = net_detect_light->forward(blob);
            
#if defined(USE_BMNN)
			std::string ext{"_Softmax"};
#else
			std::string ext{""};
#endif
			std::string out_names = {"output0" + ext};
            float* light_conf = network_result[out_names]->mutable_cpu_data();
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
            std::vector<float> cropped_result = yolo8_detect(cropped_image, (model_type_ == 0 ? 128 : 32), (model_type_ == 0 ? 128 : 32));// 灯光检测
            
            pump_light::box_info_internal ans;
            ans.light_status = (cropped_result[0]<= cropped_result[1] && con_thres <=cropped_result[1]) ?1:0;
            ans.score = cropped_result[ans.light_status];
            return exposing::make_as_first<box_info_impl>(ans);
        }
        std::string model_directory_;
        int device_;
        int model_type_;
        std::shared_ptr<GenPipeline> net_detect_light;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device, int model_type)
        : impl_{ std::make_unique<impl>(model_directory, device, model_type) }
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

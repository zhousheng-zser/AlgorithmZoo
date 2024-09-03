#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
//#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <GenPipeline/GenPipeline.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace glasssix::pump_cover_plate
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device } 
        {
            if (*model_directory_.rbegin() != '/') 
                model_directory_ += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_pump_cover_plate_cls = std::make_shared<GenPipeline>(model_directory_ + "pump_cover_plate_class.rknn", device);
#elif defined(USE_BMNN)
            net_pump_cover_plate_cls = std::make_shared<GenPipeline>(model_directory_ + "pump_cover_plate_class.bmodel", device);
#endif
            net_pump_cover_plate_cls->manual_possible_normalization(0, 1.f / 255);
        }

        pump_cover_plate::box_info detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            float x1 = param_map.count("x1") ? std::round(param_map["x1"]) : 0;
            float y1 = param_map.count("y1") ? std::round(param_map["y1"]) : 0;
            float x2 = param_map.count("x2") ? std::round(param_map["x2"]) : 0;
            float y2 = param_map.count("y2") ? std::round(param_map["y2"]) : 0;
            float x3 = param_map.count("x3") ? std::round(param_map["x3"]) : 0;
            float y3 = param_map.count("y3") ? std::round(param_map["y3"]) : 0;
            float x4 = param_map.count("x4") ? std::round(param_map["x4"]) : 0;
            float y4 = param_map.count("y4") ? std::round(param_map["y4"]) : 0;
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

            // 创建掩码图像并填充
            cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC3);
            std::vector<cv::Point> contours;
            contours.push_back(cv::Point(x1,y1));
            contours.push_back(cv::Point(x2,y2));
            contours.push_back(cv::Point(x3,y3));
            contours.push_back(cv::Point(x4,y4));
            std::vector<std::vector<cv::Point>> pts{ contours };
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            cv::fillPoly(mask, contours, cv::Scalar(255, 255, 255));
#elif defined(USE_BMNN)
            cv::fillPoly(mask, pts, cv::Scalar(255, 255, 255));
#endif
            // 通过位运算提取ROI
            cv::Mat dst;
            cv::bitwise_and(image, mask, dst);

            cv::Mat cropped_image = dst(cv::Range(mi_y, mx_y), cv::Range(mi_x, mx_x)).clone();
            pump_cover_plate::box_info result;
            result = run_detect(cropped_image, param_map);

            return std::move(result);
        }

        std::string version()
        {
            const std::string nn_frame_version = "1.0.0";

            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, "");
        }
    private:

        cv::Mat preprocess_detection(cv::Mat& src, cv::Size input_shape = cv::Size(640, 640))
        {
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
            return mask_image;
        }

        pump_cover_plate::box_info run_detect(cv::Mat& cropped_image, std::map<std::string, float>& param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.9f;

            auto new_shape = cv::Size(256, 256);
            cv::Mat blob = preprocess_detection(cropped_image, new_shape);
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;
            std::shared_ptr<memory::tensor<float>> real_forwards;

            auto network_result = net_pump_cover_plate_cls->forward(blob);
            auto temp_cls_rst = network_result.begin()->second;
            float* cls_conf = temp_cls_rst->mutable_cpu_data();

            pump_cover_plate::box_info_internal ans;
            if (conf_thres < cls_conf[0] && cls_conf[1] <= cls_conf[0])
            {
                    ans.cover_plate_status = 0; 
                    ans.score = cls_conf[0];  //不报警 关闭的 
            }
            else if (conf_thres < cls_conf[1] && cls_conf[0] <= cls_conf[1] )
            {
                    ans.cover_plate_status = 1;
                    ans.score = cls_conf[1];  //1 报警 打开的 
            }
            return exposing::make_as_first<box_info_impl>(ans);

        }
        std::string model_directory_;
        int device_;
        std::shared_ptr<GenPipeline>  net_pump_cover_plate_cls;

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

    pump_cover_plate::box_info detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

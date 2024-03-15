#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
//#include "logger.hpp"

#include <abi/param_vector.hpp>
#include <utility>

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
        impl(){}

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
            const std::string nn_frame_version = "1.1.1";

            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, "");
        }

    private:

        /**
           * @fun run_detect
           * @param image param_map
           * @details run detect (maybe in multithreading)
        */
        pump_light::box_info run_detect(cv::Mat& image, int height, int width, std::map<std::string, float>& param_map)
        {
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
            cv::Mat hsv_frame;
            cv::cvtColor(image, hsv_frame, cv::COLOR_BGR2HSV);

            std::vector<cv::Point> contours(4);//�ĵ㶨λkuang
            contours[0].x = static_cast<int>(x1);
            contours[0].y = static_cast<int>(y1);
            contours[1].x = static_cast<int>(x2);
            contours[1].y = static_cast<int>(y2);
            contours[2].x = static_cast<int>(x3);
            contours[2].y = static_cast<int>(y3);
            contours[3].x = static_cast<int>(x4);
            contours[3].y = static_cast<int>(y4);

            // ����ı�������Ӧ������
            cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC3);
            std::vector<std::vector<cv::Point>> pts{ contours };
            cv::fillPoly(mask, pts, cv::Scalar(255, 255, 255));
            cv::Mat masked_image;
            cv::bitwise_and(hsv_frame, mask, masked_image);

            // ���������������������
            cv::Mat red_mask, white_mask, orange_mask,grey_mask;
            cv::inRange(masked_image, cv::Scalar(156, 43, 46), cv::Scalar(180, 255, 255), red_mask);
            cv::inRange(masked_image, cv::Scalar(0, 0, 80), cv::Scalar(180, 43, 255), white_mask);
            cv::inRange(masked_image, cv::Scalar(11, 43, 46), cv::Scalar(25, 255, 255), orange_mask);
            cv::inRange(masked_image, cv::Scalar(0, 0, 46), cv::Scalar(180, 43, 220), grey_mask);
            int red_count = cv::countNonZero(red_mask);
            int white_count = cv::countNonZero(white_mask);
            int orange_count = cv::countNonZero(orange_mask);
            int grey_count = cv::countNonZero(grey_mask);

            // �����ı����������ʵ��������
            double total_pixels = cv::contourArea(contours);

            pump_light::box_info_internal ans;
            ans.white_ratio = white_count / total_pixels;
            ans.red_ratio = red_count / total_pixels;
            ans.orange_ratio = orange_count / total_pixels;
            ans.grey_ratio = grey_count / total_pixels;

            if ( (ans.red_ratio > 0.45 && 0.2> ans.white_ratio && ans.white_ratio > 0.05)   || ans.grey_ratio > 0.75 ||(ans.red_ratio > 0.45 && ans.orange_ratio > 0.1) )
                ans.light_status = false;
            else if ( (0.45 > ans.red_ratio && ans.red_ratio > 0.2) || ans.white_ratio > 0.3 || ans.orange_ratio > 0.2)
                ans.light_status = true;
            else
                ans.light_status = false;
            return exposing::make_as_first<box_info_impl>(ans);
        }

    };

    detect_code_internal::detect_code_internal()
        : impl_{ std::make_unique<impl>() }
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

#include "gate_status_internal.hpp"

#include <algorithm>

#include <Primitives/tensor_conversions.hpp>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::pump_gate_status
{

    class gate_status_internal::impl
    {
    public:
        impl(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) 
        {
        }

        impl() 
        {
        }

        impl(const std::vector<std::string> &phai, std::string_view racy_path, int device) 
        {
        }

        cv::Scalar hsv_parse(int int_hsv,cv::Scalar& input_hsv)
        {
            int v = int_hsv%1000;
            int_hsv = int_hsv/1000;
            int s =int_hsv%1000;
            int_hsv = int_hsv/1000;
            int h =int_hsv%1000;
                // std::cout<<h<<" "<<s<<" "<<v<<std::endl;
            input_hsv[0] = h;
            input_hsv[1] = s;
            input_hsv[2] = v;
            cv::Scalar hsv(h,s,v);
            return hsv;
        } 

        //door:  0:close or night  1:open  0:half 
        int statistic_yellow(cv::Mat img, cv::Scalar& yellow_hsv_lower, cv::Scalar& yellow_hsv_upper, double ratio_closed, double ratio_opened) 
        {
            cv::Mat HSV;
            cv::cvtColor(img, HSV, cv::COLOR_BGR2HSV);

            double sum_s = 0;
            double sum_v = 0;
            for (int i = 0; i < HSV.rows; ++i) {
                for (int j = 0; j < HSV.cols; ++j) {
                    // 访问S通道的值
                    sum_s += HSV.at<cv::Vec3b>(i, j)[1];
                    // 访问S通道的值
                    sum_v += HSV.at<cv::Vec3b>(i, j)[2];
                }
            }
            sum_s /= (HSV.rows * HSV.cols);
            sum_v /= (HSV.rows * HSV.cols);
            if (sum_s < 10 || sum_v < 30)
                return 0;

            cv::Mat mask;
            cv::inRange(HSV, yellow_hsv_lower, yellow_hsv_upper, mask);

            int count = cv::countNonZero(mask);
            int h = img.rows;
            int w = img.cols;
            int all = h * w;
            double ratio = static_cast<double>(count) / all;

            // std::cout<<"ratio: "<<ratio<<std::endl;
            if (ratio > ratio_closed) {
                return 0;
            } else if (ratio < ratio_opened) {
                return 1;
            } else {   
                return 0;
            }
        }

        int yellow_filter(cv::Mat & image, ROI& roi_door,cv::Scalar& yellow_hsv_lower, cv::Scalar& yellow_hsv_upper, double ratio_closed = 0.18, double ratio_opened = 0.02)
        {
            int x_min = roi_door.x1, x_max = roi_door.x2, y_min = roi_door.y1, y_max = roi_door.y2;
            // cv::Scalar yellow_hsv_lower(25, 51, 128);
            // cv::Scalar yellow_hsv_upper(33, 204, 255);

            cv::Mat crop_image = image(cv::Range(y_min, y_max), cv::Range(x_min, x_max));
            return statistic_yellow(crop_image, yellow_hsv_lower, yellow_hsv_upper, ratio_closed, ratio_opened);
        }

		int statistic_gray(cv::Mat img, cv::Scalar yellow_hsv_lower, cv::Scalar yellow_hsv_upper, double ratio_arrived) 
        {
            cv::Mat HSV;
            cv::cvtColor(img, HSV, cv::COLOR_BGR2HSV);

            double sum_s = 0;
            double sum_v = 0;
            for (int i = 0; i < HSV.rows; ++i) {
                for (int j = 0; j < HSV.cols; ++j) {
                    // 访问S通道的值
                    sum_s += HSV.at<cv::Vec3b>(i, j)[1];
                    // 访问S通道的值
                    sum_v += HSV.at<cv::Vec3b>(i, j)[2];
                }
            }
            sum_s /= (HSV.rows * HSV.cols);
            sum_v /= (HSV.rows * HSV.cols);
            if (sum_s < 15 || sum_v < 30)
                return 1;

            cv::Mat mask;
            cv::inRange(HSV, yellow_hsv_lower, yellow_hsv_upper, mask);

            int count = cv::countNonZero(mask);
            int h = img.rows;
            int w = img.cols;
            int all = h * w;
            double ratio = static_cast<double>(count) / all;

            if (ratio < ratio_arrived) 
            {
                return 0;
            }else 
            {
                return 1;
            }
        }

        int statistic_gray_and_line(cv::Mat& img, cv::Mat& img_full)
        {
            cv::Mat HSV;
            cv::cvtColor(img, HSV, cv::COLOR_BGR2HSV);

            double sum_s = 0;
            double sum_v = 0;
            for (int i = 0; i < HSV.rows; ++i) {
                for (int j = 0; j < HSV.cols; ++j) {
                    // 访问S通道的值
                    sum_s += HSV.at<cv::Vec3b>(i, j)[1];
                    // 访问S通道的值
                    sum_v += HSV.at<cv::Vec3b>(i, j)[2];
                }
            }
            sum_s /= (HSV.rows * HSV.cols);
            sum_v /= (HSV.rows * HSV.cols);
            if (sum_s < 15 || sum_v < 30)
                return 1;

            cv::Mat crop_image = img_full(cv::Range(935, 1070), cv::Range(1124, 1198));
            cv::Mat gray;
            cv::cvtColor(crop_image, gray, cv::COLOR_BGR2GRAY);
            cv::Mat edges;

            cv::Mat hist, blur;
            // 直方图均衡化
            cv::equalizeHist(gray, hist);
            // 高斯模糊
            cv::GaussianBlur(hist, blur, cv::Size(9, 9), 2);

            cv::Canny(blur, edges, 50, 150, 3);
            std::vector<cv::Vec4i> points;

            cv::HoughLinesP(edges, points, 1, CV_PI / 180, 40, 40, 10);
            int line = 0;
            for (int i = 0; i < points.size(); i++) {
                int x1, x2, y1, y2;
                x1 = points[i][0];
                y1 = points[i][1];
                x2 = points[i][2];
                y2 = points[i][3];
                double slope = 1.0 * (y2 - y1) / (x2 - x1);
                if (slope >= 1.7 && slope <= 2.7)
                    ++line;
            }
            if (line > 0)
                return 1;
            return 0;
        }

        int gray_filter( cv::Mat &image,ROI& roi_door, cv::Scalar& gray_hsv_lower, cv::Scalar& gray_hsv_upper, int device_id, double ratio_arrived=0.18 ) //0 :有车      1: 没车
        {
			int x_min = roi_door.x1, x_max = roi_door.x2, y_min = roi_door.y1, y_max = roi_door.y2;

            cv::Mat crop_image = image(cv::Range(y_min, y_max), cv::Range(x_min, x_max));
            if(device_id!= 10)
                return statistic_gray(crop_image, gray_hsv_lower, gray_hsv_upper, ratio_arrived);
            return statistic_gray_and_line(crop_image, image);
        }

        int gate_status_detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int yellow_hsv_lower, int yellow_hsv_upper, int gray_hsv_lower, int gray_hsv_upper,
            std::vector<int>& rois,
            std::map<std::string, float>& param_map_abi)
        {

            double door_close_ratio = param_map_abi.count("door_close_ratio") ? param_map_abi["door_close_ratio"] : 0.18;
            double door_open_ratio = param_map_abi.count("door_open_ratio") ? param_map_abi["door_open_ratio"] : 0.02;
            double floor_ratio = param_map_abi.count("floor_ratio") ? param_map_abi["floor_ratio"] : 0.18;
            int device_id = param_map_abi.count("device_id") ? std::round(param_map_abi["device_id"]) : 0;

            if (device_id < 10 || device_id > 17)
                throw exposing::abi_invalid_argument("pump_gate_status: Invalid device_id");

            cv::Scalar yellow_lower;
            cv::Scalar yellow_upper;
            cv::Scalar gray_lower;
            cv::Scalar gray_upper;
            hsv_parse(yellow_hsv_lower, yellow_lower);
            hsv_parse(yellow_hsv_upper, yellow_upper);
            hsv_parse(gray_hsv_lower, gray_lower);
            hsv_parse(gray_hsv_upper, gray_upper);

            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(channels, 3);
            CHECK_EQ(rois.size(), 8);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            ROI door(rois[0], rois[1], rois[2], rois[3]);
            ROI floor(rois[4], rois[5], rois[6], rois[7]);

            bool  opened_door = yellow_filter(image, door, yellow_lower, yellow_upper, door_close_ratio, door_open_ratio) != 0;//0为关闭
            if (!opened_door)
                return 0;
            if (device_id != 10)
                return opened_door;
            else {
                bool  working = gray_filter(image, floor, gray_lower, gray_upper, device_id, floor_ratio) == 0; //0为有东西
                return opened_door && working;
            }
        }


    private:
        int device_;

    };


    gate_status_internal::gate_status_internal():impl_{std::make_unique<impl>()}
    {
    }

    gate_status_internal::gate_status_internal(exposing::param_vector<int> hsvs)
    {
    }

    gate_status_internal::gate_status_internal(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) : 
    impl_{std::make_unique<impl>(model_type, racy_path, device, use_int8)}
    {
    }

    gate_status_internal::gate_status_internal(const std::vector<std::string> &phai, std::string_view racy_path, int device) : impl_{std::make_unique<impl>(phai, racy_path, device)}
    {
    }

    gate_status_internal::~gate_status_internal()
    {
    }

    std::vector<std::vector<float>> gate_status_internal::get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order) const
    {
        std::vector<std::vector<float>> temp;
        return temp;
    }


    int gate_status_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int yellow_hsv_lower, int yellow_hsv_upper, int gray_hsv_lower, int gray_hsv_upper, 
            std::vector<int>& rois,
            std::map<std::string, float>&  param_map_abi) const
    {
        return impl_->gate_status_detect(bitmap, channels, height, width, yellow_hsv_lower, yellow_hsv_upper, gray_hsv_lower,  gray_hsv_upper, rois,  param_map_abi  );
    }

    std::string gate_status_internal::version()
    {

        const std::string nn_frame_version = "1.4.0";

        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version,"");
    }
}

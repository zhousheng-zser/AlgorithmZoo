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

namespace glasssix::subway_anomaly
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device } 
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_detect_subway = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("subway_anomaly", false),
                std::string(model_directory) + "/" + "subway_anomaly.rknn", device);

#else
            net_detect_subway = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("subway_anomaly", false),
                std::string(model_directory) + "/" + "subway_anomaly.racy", device);
#endif  
            init_data_compatible(640, 640, add_weight_subway, mul_weight_subway);
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

        subway_anomaly::box_info detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in subway_anomaly");
            }
            int type = std::round(param_map.count("type") ? param_map["type"] : 0.0 );

            subway_anomaly::box_info result;
            if (type == 0)
            {
                cv::Mat cropped_image = image.clone();
                cv::Rect compare_area(roi_x, roi_y, roi_width, roi_height); // x, y, width, height
                result = run_detect_nzx(cropped_image, compare_area , param_map);  //宁梓骁的
            }
            else {
                cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();
                result = run_detect_yf(cropped_image, param_map);    //杨凡的
            }

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

            auto network_result = net_detect_subway->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
            float* subway_conf = network_result["output0"]->mutable_cpu_data();
            std::vector<float> current_frame_result;
            current_frame_result.push_back(subway_conf[0]);
            current_frame_result.push_back(subway_conf[1]);
            return current_frame_result;

        }

    private:

        /**
           * @fun run_detect
           * @param image param_map
           * @details run detect (maybe in multithreading)
        */
        subway_anomaly::box_info run_detect_yf(cv::Mat& cropped_image, std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.9f;
            std::vector<float> cropped_result = yolo8_detect(cropped_image, 640, 640);
            subway_anomaly::box_info_internal ans;
            if (cropped_result[0] >= cropped_result[1] && cropped_result[0] >=0.9)
            {
                ans.anomaly_status = 0;   //报警
                ans.score = cropped_result[0];
            }
            else
            {
                ans.anomaly_status = 1;
                ans.score = cropped_result[1];  //不报警
            }
            return exposing::make_as_first<box_info_impl>(ans);
        }
        subway_anomaly::box_info run_detect_nzx(cv::Mat& image, cv::Rect& compare_area, std::map<std::string, float>& param_map)
        {
            float normal_closedoor_thresh = param_map.count("normal_closedoor_thresh") ? param_map["normal_closedoor_thresh"] : 0.05;
            cv::Mat thresh;
            cv::threshold(image(compare_area), thresh, 150, 255, cv::THRESH_BINARY);
            cv::Mat black_pixels;
            cv::inRange(thresh, cv::Scalar(0), cv::Scalar(0), black_pixels);
            double num_black_pixels = cv::countNonZero(black_pixels);
            double total_pixels = thresh.total();
            double black_pixel_ratio = num_black_pixels / total_pixels;
            double notclosed_ratio = std::fabs(black_pixel_ratio * 100 - 71) / 71;
            subway_anomaly::box_info_internal ans;
            if (notclosed_ratio  >= normal_closedoor_thresh) {
                ans.anomaly_status = 0;   //报警
                ans.score = notclosed_ratio;
            }
            else {
                ans.anomaly_status = 1;   //报警
                ans.score = notclosed_ratio;
            }
            return exposing::make_as_first<box_info_impl>(ans);
        }
        std::string model_directory_;
        int device_;
        std::vector<float> add_weight_subway;
        std::vector<float> mul_weight_subway;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_subway;  
#else
        std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_subway;
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

    subway_anomaly::box_info detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}

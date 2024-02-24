#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <chrono>
#include <chrono>
// #include <opencv2/highgui.hpp>
// #include <opencv2/core.hpp>
// #include <opencv2/imgproc.hpp>
// #include <opencv2/dnn.hpp>
// #include "hardcode.hpp"
// #include "Excalibur/pipeline.hpp"
// #include "Primitives/tensor_conversions.hpp"

#include "general.hpp"

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>

namespace glasssix::pump_mask
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_detect_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("pump_mask", false),
                std::string(model_directory) + "/" + "pump_mask.rknn", device);

            /*            net_classify_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("pump_mask", false),
                        std::string(model_directory) + "/" +"pump_mask_class.rknn", device); */
#else
            net_detect_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("pump_mask", false),
                std::string(model_directory) + "/" + "pump_mask.racy", device);

            /*            net_classify_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("pump_mask", false),
                        std::string(model_directory) + "/" +"pump_mask_class.racy", device); */
#endif  
            init_data();
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string nn_frame_version = net_detect_->version();
#else
            std::string nn_frame_version = net_detect_->version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

        void init_data()
        {
            for (size_t i = 0; i < 33600; i++)
            {
                if (i < 25600)
                {
                    add_weight[i] = i % 160;
                    add_weight[i + 33600] = i / 160;
                    mul_weight[i] = 8.f;
                }
                else if (i < 32000)
                {
                    add_weight[i] = (i - 25600) % 80;
                    add_weight[i + 33600] = (i - 25600) / 80;
                    mul_weight[i] = 16.f;
                }
                else
                {
                    add_weight[i] = (i - 32000) % 40;
                    add_weight[i + 33600] = (i - 32000) / 40;
                    mul_weight[i] = 32.f;
                }
            }
        }

        std::vector<Bbox> yolo_detect(cv::Mat& image, float conf_threshold, float iou_threshold)
        {
            auto new_shape = cv::Size(1280, 1280);
            cv::Mat blob;
            float ratio = 0;
            int pad_h = 0;
            int pad_w = 0;
            std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            std::shared_ptr<memory::tensor<float>> real_forwards;
            auto  network_result = net_detect_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names = { "845","844","843" };

            for (size_t i = 0; i < out_names.size(); i++)
            {
                forwards.push_back(network_result[out_names[i]]);
            }

            int candicate_num = 0;
            std::vector<int> class_mask;
            auto real_output = Yolov8s_Concat(forwards, conf_threshold, candicate_num, add_weight.data(), mul_weight.data(), class_mask);

            auto nms_input640 = XYXY2WH(real_output, pad_h, pad_w, 1.f / ratio, candicate_num, class_mask);

            box_result_move_to_disjoint_region(nms_input640, class_mask, 100000);

            auto nms_result_index = nms_process(nms_input640, conf_threshold, iou_threshold);

            box_result_move_to_disjoint_region(nms_input640, class_mask, -100000);

            std::vector<Bbox> current_frame_result;

            for (size_t i = 0; i < nms_result_index.size(); i++)
            {
                int index = nms_result_index[i];
                current_frame_result.emplace_back(nms_input640[index][0], nms_input640[index][1], nms_input640[index][0] + nms_input640[index][2], nms_input640[index][1] + nms_input640[index][3], class_mask[index], nms_input640[index][4], 0);
            }
            return current_frame_result;

        }

        std::vector<std::vector<cv::Point>> grab_contours(std::vector<std::vector<cv::Point>>& cnts) {
            // 从传入的向量中提取轮廓
            std::vector<std::vector<cv::Point>> contours;
            for (size_t i = 0; i < cnts.size(); ++i) {
                contours.push_back(cnts[i]);
            }
            return contours;
        }
        // 自定义函数，用于将 (x, y, w, h) 格式转换为 (x1, y1, x2, y2) 格式
        std::vector<cv::Rect> convert_xywh_to_xyxy(const std::vector<cv::Rect>& xywh_list) {
            std::vector<cv::Rect> xyxy_list;
            for (const auto& xywh : xywh_list) {
                int x1 = xywh.x;
                int y1 = xywh.y;
                int x2 = xywh.x + xywh.width;
                int y2 = xywh.y + xywh.height;
                xyxy_list.push_back(cv::Rect(x1, y1, x2, y2));
            }
            return xyxy_list;
        }

        // 判断两点之间的距离是否有效
        bool judge_point_valid(const cv::Point& a, const cv::Point& b, double min_distance, double max_distance) {
            return min_distance <= abs(a.x - b.x) && abs(a.x - b.x) <= max_distance && min_distance <= abs(b.y - a.y)
                && abs(b.y - a.y) <= max_distance;
        }

        // 计算两点之间的斜率
        double get_slope_from_points(const cv::Point& p1, const cv::Point& p2) {
            return (p2.y - p1.y) / (p2.x - p1.x + 0.00001);
        }

        // 比较两个斜率是否相近
        bool is_close_slope(double s1, double s2, double tolerance) {
            return std::abs(s1 - s2) <= tolerance * std::max(std::abs(s1), std::abs(s2));
        }

        //获取合法线段数量
        int get_line_list_num(std::vector<cv::Point>& point_list, int min_parallel_line) {
            std::sort(point_list.begin(), point_list.end(), [](cv::Point a, cv::Point b) {
                if (a.x == b.x)
                return a.y < b.y;
            return a.x < b.x;
                });
            std::vector<std::vector<cv::Point>> line_list;
            int n = point_list.size();
            double min_distance = 3, max_distance = 50;
            double tolerance = 0.3;
            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    if (!judge_point_valid(point_list[i], point_list[j], min_distance, max_distance)) {
                        continue;
                    }

                    std::vector<cv::Point> line = { point_list[i], point_list[j] };
                    for (int k = j + 1; k < n; ++k) {
                        cv::Point p = point_list[k];

                        if (std::abs((p.x - point_list[j].x) - (point_list[j].x - point_list[i].x)) > 5
                            || std::abs((p.y - point_list[j].y) - (point_list[j].y - point_list[i].y)) > 5) {
                            continue;
                        }

                        double s1 = (point_list[j].y - point_list[i].y) * (p.x - point_list[j].x);
                        double s2 = (p.y - point_list[j].y) * (point_list[j].x - point_list[i].x);
                        if (!is_close_slope(s1, s2, tolerance)) {
                            continue;
                        }

                        line.push_back(p);
                        break;
                    }
                    if (line.size() == 3) {
                        line_list.push_back(line);
                    }
                }
            }

            // 计算斜率并过滤
            std::vector<double> k_list;
            for (const auto& line : line_list) {
                double slope = get_slope_from_points(line[0], line[2]);
                k_list.push_back(std::round(slope));
            }
            std::map<double, int> counter;
            for (const auto& k : k_list) {
                counter[k]++;
            }

            std::vector<double> valid_k_list;
            for (const auto& item : counter) {
                if (item.second >= min_parallel_line) {
                    valid_k_list.push_back(item.first);
                }
            }

            std::vector<std::vector<cv::Point>> valid_line_list;
            for (size_t idx = 0; idx < k_list.size(); ++idx) {
                if (std::find(valid_k_list.begin(), valid_k_list.end(), k_list[idx]) != valid_k_list.end()) {
                    valid_line_list.push_back(line_list[idx]);
                }
            }
            return valid_line_list.size();

        }

        //判断工作状态
        bool work_status(cv::Mat& image) {
            int image_height = image.rows;
            int image_width = image.cols;
            int x, y, w, h;
            x = image_width * 5 / 12;
            y = image_height * 5 / 6;
            w = image_width / 3;
            h = image_height / 6;

            cv::Rect cut(x, y, w, h);
            cv::Mat cropped_image = image(cut).clone();
            image_height = cropped_image.rows;
            image_width = cropped_image.cols;
            cv::Mat gray_image;
            cv::cvtColor(cropped_image, gray_image, cv::COLOR_BGRA2GRAY);
            cv::Mat blurred_image;
            cv::GaussianBlur(gray_image, blurred_image, cv::Size(5, 5), 0, 0);
            cv::Mat threshold_image;
            double threshold = cv::threshold(blurred_image, threshold_image, 50, 255, cv::THRESH_BINARY);
            cv::Mat kernel_image = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(18, 18));
            cv::Mat black_hat_image;
            cv::morphologyEx(threshold_image, black_hat_image, cv::MORPH_BLACKHAT, kernel_image);
            cv::Mat close_image;
            cv::morphologyEx(
                black_hat_image, close_image, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)));

            int MIN_HOLE_AREA = 5 * 5;
            int MAX_HOLE_AREA = 20 * 20;
            int MAX_HOLE_W_H_RATIO = 2;
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(close_image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            contours = grab_contours(contours);

            // 将每个轮廓转换为边界框 (x, y, w, h)
            std::vector<cv::Rect> xywh_list;
            for (const auto& cnt : contours) {
                xywh_list.push_back(cv::boundingRect(cnt));
            }

            // 根据 MIN_HOLE_AREA 和 MAX_HOLE_AREA 条件过滤边界框
            std::vector<cv::Rect> filtered_xywh_list;
            for (const auto& xywh : xywh_list) {
                int area = xywh.width * xywh.height;
                if (area >= MIN_HOLE_AREA && area <= MAX_HOLE_AREA) {
                    filtered_xywh_list.push_back(xywh);
                }
            }

            // 根据 MAX_HOLE_W_H_RATIO 条件过滤边界框
            std::vector<cv::Rect> final_xywh_list;
            for (const auto& xywh : filtered_xywh_list) {
                double aspect_ratio =
                    static_cast<double>(std::max(xywh.width, xywh.height)) / std::min(xywh.width, xywh.height);
                if (aspect_ratio <= MAX_HOLE_W_H_RATIO) {
                    final_xywh_list.push_back(xywh);
                }
            }

            // 将边界框格式转换为 (x1, y1, x2, y2)
            std::vector<cv::Rect> xyxy_list = convert_xywh_to_xyxy(final_xywh_list);

            std::vector<cv::Point> line;
            for (int i = 0; i < xyxy_list.size(); i++) {
                line.push_back(cv::Point(xyxy_list[i].x + xyxy_list[i].width >> 1, xyxy_list[i].y + xyxy_list[i].height >> 1));
            }

            int ans = get_line_list_num(line, 4);
            return ans >= 10 ? true : false;
        }

        inline int ComputeArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
            int x = std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1));
            int y = std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
            return x * y;
        }

        exposing::param_vector<pump_mask::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.3f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (!work_status(image)) {
                return {};
            }

            std::vector<Bbox> frame_result = yolo_detect(image, con_thres, iou_thres);
            std::vector<Bbox> person_box_list;
            std::vector<Bbox> head_box_list;

            for (Bbox& temp : frame_result) {
                if (temp.category == 0)
                    person_box_list.push_back(temp);
                else if (temp.x2 - temp.x1 > 20 && temp.y2 - temp.y1 > 20)
                    head_box_list.push_back(temp);
            }
            std::vector<box_info_internal> result;
            for (int i = 0; i < head_box_list.size(); ++i) {
                for (int j = 0; j < person_box_list.size(); ++j)
                {
                    int ax1 = head_box_list[i].x1;
                    int ay1 = head_box_list[i].y1;
                    int ax2 = head_box_list[i].x2; 
                    int ay2 = head_box_list[i].y2; 
                    int bx1 = head_box_list[j].x1; 
                    int by1 = head_box_list[j].y1; 
                    int bx2 = head_box_list[j].x2; 
                    int by2 = head_box_list[j].y2;
                    if (ComputeArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2) / ((ay2 - ay1) * (ax2 - ax1)) >= 0.99999) {
                        box_info_internal temp_result;
                        temp_result.x1 = head_box_list[i].x1;
                        temp_result.y1 = head_box_list[i].y1;
                        temp_result.x2 = head_box_list[i].x2;
                        temp_result.y2 = head_box_list[i].y2;
                        temp_result.category = head_box_list[i].category;
                        temp_result.score = head_box_list[i].score;
                        result.push_back(temp_result);
                        break;
                    }
                }
            }
            auto fin_result = exposing::make_param_vector<pump_mask::box_info>();
            for (auto& i : result)
            {
                fin_result.push_back(exposing::make_as_first<box_info_impl>(i));
            }

            return fin_result;
        }

    private:
        std::string model_directory_;
        int device_;
        std::array<float, 33600 * 2> add_weight;
        std::array<float, 33600>   mul_weight;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_;
        //std::unique_ptr < rknnwrapper::rknn_wrapper> net_classify_;    
#else
        std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_;
        //std::unique_ptr < glasssix::excalibur::pipeline<float>> net_classify_;  
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

    exposing::param_vector<pump_mask::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, param_map);
    }
}

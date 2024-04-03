#include "classify_code_internal.hpp"
#include "box_info_internal.hpp"
#include "box_info_impl.hpp"

#include <algorithm>
#include <numeric>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

// #include "dbg.h"

namespace glasssix::vehicle
{
    class classify_code_internal::impl
    {
    public:
        impl() {}
        impl(std::string_view model_directory, int device) : impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/')
                model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "vehicle.rknn", 0);
#elif defined(USE_BMNN)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "vehicle.bmodel", 0);
#else
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "vehicle.onnx", 0);
#endif
            ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<1, 0>);
        }

        struct VehicleBox : public GenPipTools::YoloBoxBase
        {
        public:
            using YoloBoxBase::YoloBoxBase; // Inheriting Constructors
        };

        exposing::param_vector<vehicle::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float> &param_map_std)
        {
            auto result = exposing::make_param_vector<vehicle::box_info>();
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in universal_pedestrian");
            }
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t *>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            std::vector<VehicleBox> vehicle_list = run_detect(cropped_image, param_map_std);

            for (auto &vehicle : vehicle_list)
            {
                box_info_internal vehicle_internal;
                vehicle.add(roi_x, roi_y);
                std::int32_t x1, y1, x2, y2;
                x1 = vehicle.xmin;
                y1 = vehicle.ymin;
                x2 = vehicle.xmax;
                y2 = vehicle.ymax;
                //& 画原始的框
                // cv::rectangle(image, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 3);
                // cv::imwrite("last" + std::to_string(0) + ".jpg", image);

                {
                    // 判定区域是否合法
                    if (x1 > x2 || y1 > y2)
                        continue;
                    x1 = std::max(0, x1);
                    x1 = std::min(width, x1);

                    x2 = std::max(0, x2);
                    x2 = std::min(width, x2);

                    y1 = std::max(0, y1);
                    y1 = std::min(height, y1);

                    y2 = std::max(0, y2);
                    y2 = std::min(height, y2);
                }
                std::vector<std::vector<int>> points;
                {
                    // 获取车辆经过划线得到的多边形(目前是5个顶点)
                    cv::Rect roi_rect{x1, y1, x2 - x1, y2 - y1};
                    // cv::Mat roi_img = image(roi_rect).clone();
                    // int img_w = roi_img.cols;
                    // int img_h = roi_img.rows;
                    // cv::Mat hsv;
                    // cv::cvtColor(roi_img.clone(),hsv,cv::COLOR_BGR2HSV);
                    points = vehicle_border_detect(image.clone(), roi_rect);
                    std::vector<cv::Point> Points = {cv::Point(points[0][0] + x1, points[0][1] + y1), cv::Point(points[1][0] + x1, points[1][1] + y1), cv::Point(points[2][0] + x1, points[2][1] + y1), cv::Point(points[3][0] + x1, points[3][1] + y1), cv::Point(points[4][0] + x1, points[4][1] + y1)};
                    //& 画多边形
                    // cv::polylines(image, Points, true, cv::Scalar(0, 255, 0), 1);
                    // cv::imwrite("last" + std::to_string(1) + ".jpg", image);
                }
                vehicle_internal.x1 = points[0][0] + x1;
                vehicle_internal.y1 = points[0][1] + y1;
                vehicle_internal.x2 = points[1][0] + x1;
                vehicle_internal.y2 = points[1][1] + y1;
                vehicle_internal.x3 = points[2][0] + x1;
                vehicle_internal.y3 = points[2][1] + y1;
                vehicle_internal.x4 = points[3][0] + x1;
                vehicle_internal.y4 = points[3][1] + y1;
                vehicle_internal.x5 = points[4][0] + x1;
                vehicle_internal.y5 = points[4][1] + y1;
                vehicle_internal.score = vehicle.score;
                vehicle_internal.category = 0;
                result.push_back(exposing::make_as_first<box_info_impl>(vehicle_internal));
            }
            return result;
        }

        std::vector<VehicleBox> run_detect(cv::Mat &image, std::map<std::string, float> &param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.7f;
            constexpr int infrW = 640;
            constexpr int infrH = 640;
            constexpr bool ifCvtRGB = true;
            GenPipTools::LetterInfo letter_op;

            auto letter_img = GenPipTools::letter_image(image, infrW, infrH, letter_op, ifCvtRGB);
            auto net_rstmap = ioprocess_pipeline_->forward(letter_img);
            auto tensor_out = net_rstmap.begin()->second;
            const int vf_nums = tensor_out->height(); // vf, visual field
            const int per_vf_len = tensor_out->width();
            std::vector<VehicleBox> box_list;
            for (size_t idx = 0; idx < vf_nums; idx++)
            {
                float *pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float conf_pos = pdata[4];
                if (conf_pos > conf_thres)
                {
                    VehicleBox obj_box(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, conf_pos, 0);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, nms_thres);
            GenPipTools::letter_map_origin_location(box_list, letter_op);
            return box_list;
        }

        std::string version()
        {
            const std::string algo_module_version = "2.1.1";
            std::string nn_frame_version = ioprocess_pipeline_->version();
            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }
        // 输出五边形对应的函数
        cv::Mat get_color_mask(cv::Mat image, int image_w, int image_h)
        {
            cv::Mat hsv;
            cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
            cv::Mat mask = cv::Mat::zeros(image_h, image_w, CV_8U);
            cv::Scalar lowerColor = cv::Scalar(35, 43, 30);
            cv::Scalar upperColor = cv::Scalar(99, 255, 255);
            cv::Mat color_range;
            cv::inRange(hsv, lowerColor, upperColor, color_range);
            cv::bitwise_or(mask, color_range, mask);
            return mask;
        }

        cv::Rect convert_xywh_to_xyxy(cv::Rect xywh)
        {
            int x1 = xywh.x;
            int y1 = xywh.y;
            int x2 = xywh.x + xywh.width;
            int y2 = xywh.y + xywh.height;
            return cv::Rect(x1, y1, x2 - x1, y2 - y1);
        }

        cv::Mat filter_lower_left_corner_mask(cv::Mat image, int image_w, int image_h)
        {
            double max_box_width = image_w / 2.0;
            double max_box_height = image_h / 2.0;
            std::vector<std::vector<cv::Point>> cnts;
            cv::findContours(image, cnts, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            std::vector<cv::Rect> xywh_list;
            for (auto cnt : cnts)
            {
                cv::Rect rect = cv::boundingRect(cnt);
                xywh_list.push_back(rect);
            }
            xywh_list.erase(std::remove_if(xywh_list.begin(), xywh_list.end(),
                                           [max_box_width, max_box_height](cv::Rect xywh)
                                           {
                                               return xywh.width >= max_box_width && xywh.height >= max_box_height;
                                           }),
                            xywh_list.end());
            std::vector<cv::Rect> xyxy_list;
            for (auto xywh : xywh_list)
            {
                cv::Rect xyxy = convert_xywh_to_xyxy(xywh);
                if (xyxy.x == 0 && xyxy.br().y == image_h)
                {
                    xyxy_list.push_back(xyxy);
                }
            }
            cv::Mat filter_image;
            cv::Mat the_mask = cv::Mat::zeros(image_h, image_w, CV_8U);
            if (!xyxy_list.empty())
            {
                cv::Rect xyxy = xyxy_list[0];
                the_mask(cv::Rect(xyxy.tl(), xyxy.br())) = 1;
                cv::bitwise_and(image, image, filter_image, the_mask);
            }
            else
            {
                filter_image = the_mask;
            }
            return filter_image;
        }

        cv::Vec2f get_line(cv::Mat canny)
        {
            std::vector<cv::Vec2f> lines;
            cv::HoughLines(canny, lines, 10, CV_PI / 180, 30);
            if (lines.empty())
            {
                return cv::Vec2f(0, 0);
            }
            float min_degree = 100.0 / 180 * CV_PI;
            float max_degree = 170.0 / 180 * CV_PI;
            for (int i = 0; i < lines.size(); i++)
            {
                float rho = lines[i][0];
                float theta = lines[i][1];
                if (theta > min_degree && theta < max_degree && rho > 0)
                {
                    continue;
                }
                else
                {
                    lines.erase(lines.begin() + i);
                    i--;
                }
            }
            if (lines.empty())
            {
                return cv::Vec2f(0, 0);
            }
            std::sort(lines.begin(), lines.end(), [](const cv::Vec2f &a, const cv::Vec2f &b)
                      { return a[1] > b[1]; });
            return lines[0];
        }

        std::vector<std::vector<int>> get_box_by_line(cv::Vec2f line, int image_w, int image_h)
        {
            if (line == cv::Vec2f(0, 0))
            {
                return {{0, 0}, {image_w, 0}, {image_w, image_h}, {0, image_h}, {0, 0}, {0, 0}};
            }

            float rho = line[0];
            float theta = line[1] - CV_PI / 2;
            float y = rho * 1 / std::cos(theta);
            float x = (image_h - y) * 1 / std::tan(theta);
            x = std::max(0.0f, std::min(x, static_cast<float>(image_w / 2.0)));
            y = std::max(0.0f, std::min(y, static_cast<float>(image_h / 2.0)));

            return {{0, 0}, {image_w, 0}, {image_w, image_h}, {static_cast<int>(x), image_h}, {0, static_cast<int>(y)}, {0, 0}};
        }

        std::vector<std::vector<int>> vehicle_border_detect(cv::Mat origin_image, cv::Rect detect_rect, std::string show_image_path = "")
        {
            cv::Mat cut_image = origin_image(detect_rect);
            int image_h = cut_image.rows;
            int image_w = cut_image.cols;

            cv::Mat mask = get_color_mask(cut_image.clone(), image_w, image_h);

            cv::Mat open;
            cv::morphologyEx(mask, open, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11, 11)));

            cv::Mat filter_open = filter_lower_left_corner_mask(open, image_w, image_h);

            cv::Mat canny;
            cv::Canny(filter_open, canny, 30, 150);
            //& 画每个阶段的图片
            // cv::imwrite("last_0_origin_image.jpg", origin_image);
            // cv::imwrite("last_1_cut_image.jpg", cut_image);
            // cv::imwrite("last_2_mask.jpg", mask);
            // cv::imwrite("last_3_open.jpg", open);
            // cv::imwrite("last_4_filter_open.jpg", filter_open);
            // cv::imwrite("last_5_canny.jpg", canny);

            cv::Vec2f line = get_line(canny);

            std::vector<std::vector<int>> new_box = get_box_by_line(line, image_w, image_h);
            return new_box;
        }

    private:
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{std::make_unique<impl>(model_directory, device)}
    {
    }

    classify_code_internal::~classify_code_internal()
    {
    }

    exposing::param_vector<vehicle::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float> &param_map_std)
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    }

    std::string classify_code_internal::version()
    {
        return impl_->version();
    }
}

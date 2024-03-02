#include "detect_code_internal.hpp"
#include "box_info_internal.hpp"

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
#include "box_info_impl.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::pump_pumptop_person
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
        {
        }

        exposing::param_vector<pump_pumptop_person::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, const std::vector<PedestrianInfo>& pedestrian_info_list, std::map<std::string,float>& param_map_std)
        {
            auto result = exposing::make_param_vector<pump_pumptop_person::box_info>();

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(bitmap.size(), 3 * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3);

            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * 3 * height * width);

            constexpr int crop_x1 = 724;
            constexpr int crop_y1 = 205;
            constexpr int crop_x2 = 1444;
            constexpr int crop_y2 = 1040;

			float person_area_ratio_thres = param_map_std.count("person_area_ratio_thres") ? param_map_std["person_area_ratio_thres"] : 0.4f;

            auto [maskpump_crop, pump_rrects_crop] = pump_detect(image, crop_x1, crop_y1, crop_x2, crop_y2);
            
            for (cv::RotatedRect pump_rrect_crop : pump_rrects_crop) {

                std::vector<cv::Point2f> pump_polygon_crop = RotatedRect2Polygon(pump_rrect_crop);

                for (auto pedestrian_info : pedestrian_info_list) {
					auto person_box_crop = pedestrian_info.get_rect();
                    person_box_crop.x -= crop_x1;
                    person_box_crop.y -= crop_y1;

                    auto low_person_point = cv::Point((pedestrian_info.x1 + pedestrian_info.x2) / 2, pedestrian_info.y2);
                    auto low_person_point_crop = low_person_point - cv::Point(crop_x1, crop_y1);

                    bool is_person_in_pump_regions = isPointInsidePolygon(low_person_point_crop, pump_polygon_crop);
                    auto is_person_maskpump = maskpump_crop.at<uchar>(low_person_point_crop) == 255;
                    
					box_info_internal person_state;
					person_state.x1 = pedestrian_info.x1;
					person_state.y1 = pedestrian_info.y1;
					person_state.x2 = pedestrian_info.x2;
					person_state.y2 = pedestrian_info.y2;
					person_state.score = pedestrian_info.score;
					person_state.pump = exposing::make_param_vector<std::int32_t>();

					// install pump location
					for (auto pump_point_crop : pump_polygon_crop) {
						person_state.pump.push_back(int(pump_point_crop.x + crop_x1));
						person_state.pump.push_back(int(pump_point_crop.y + crop_y1));
					}

                    constexpr int OUT_PUMP = 0;//不在泵内
                    constexpr int IN_PUMP = 1;//在泵（均在在四边形内和掩码内）
                    constexpr int OUT_MASK = 2;//在四边形内，但不在掩码内
                    constexpr int MAN_INTERSECT_AREA_LITTE = 3;//人和泵框相交度不足，认为不在泵内

                    person_state.category = OUT_PUMP; // 默认设置为不在泵
                    if (is_person_in_pump_regions) {
                        person_state.category = (is_person_maskpump) ? IN_PUMP : OUT_MASK;
                    }

					if (person_state.category == IN_PUMP) {
                        auto intersection_area = get_intersection_area(person_box_crop, pump_rrect_crop);
                        float in_area_ratio = intersection_area / person_box_crop.area();

                        //dbg(person_box_crop);
                        //dbg(pump_rrect_crop.angle);
                        //dbg(pump_rrect_crop.center);
                        //dbg(pump_rrect_crop.size);
                        //dbg(intersection_area);
                        //dbg(in_area_ratio);
                        //dbg(person_area_ratio_thres);
                        if (in_area_ratio < person_area_ratio_thres) {
                            person_state.category = MAN_INTERSECT_AREA_LITTE;
                        }
                    }

                    result.push_back(glasssix::exposing::make_as_first<box_info_impl>(person_state));
                }
            }

            return result;
        }


        inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
        {
            int width = roi.width;
            int height = roi.height;
            int x = roi.x;
            int y = roi.y;

            cv::Mat mat(height, width, img.type(), cv::Scalar(0));
            int _x = x;
            int _y = y;
            int _width = width;
            int _height = height;
            if (x < 0)
            {
                _x = 0;
                _width = width + x;
            }

            if (_x + _width > img.cols)
                _width = img.cols - _x;

            if (y < 0)
            {
                _y = 0;
                _height = height + y;
            }

            if (_y + _height > img.rows)
                _height = img.rows - _y;

            img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
            return mat;
        }
        cv::RotatedRect top_left_scale_quadrilateral(const cv::RotatedRect& originalRect, double widthFactor, double heightFactor) {

            cv::Point2f vertices[4];
            originalRect.points(vertices);

            cv::Point2f topLeft = vertices[0];
            cv::Point2f topRight = vertices[0];
            cv::Point2f botLeft = vertices[0];
            for (int i = 1; i < 4; ++i) {
                if (topLeft.x > vertices[i].x) {
                    topLeft = vertices[i];
                }
                if (topRight.y > vertices[i].y) {
                    topRight = vertices[i];
                }
                if (botLeft.y < vertices[i].y) {
                    botLeft = vertices[i];
                }
            }

            auto scalePointTowards = [](const cv::Point2f& from, const cv::Point2f& to, float scale)
            {
                cv::Point2f direction = to - from;
                float distance = std::sqrt(direction.dot(direction));
                direction /= distance; // Normalize the direction vector  
                cv::Point2f scaledPoint = from + direction * (distance * scale);
                return scaledPoint;
            };

            // new topRight botLeft
            topRight = scalePointTowards(topLeft, topRight, widthFactor);
            botLeft = scalePointTowards(topLeft, botLeft, heightFactor);

            if ((topRight.x - topLeft.x) * (botLeft.y - topLeft.y) == (botLeft.x - topLeft.x) * (topRight.y - topLeft.y)) {
                // Three points collinear, can't determine new rectangle, Return RotatedRect.
                return originalRect;
            }

            // distance topLeft -> topRight
            cv::Point2f V_ab = topRight - topLeft;
            // distance topLeft -> botLeft
            cv::Point2f V_ac = botLeft - topLeft;
            // start add vec plus
            cv::Point2f newBotRight = topLeft + V_ab + V_ac;

            std::vector<cv::Point2f> points = { topLeft, topRight, botLeft, newBotRight };
            return cv::minAreaRect(points);
        }

        std::unordered_map<std::string, std::pair<cv::Scalar, cv::Scalar>> hsv_dict{
            {"red1", {{  0,100,100},{  7, 255, 255}}},
            {"red2", {{156, 43, 46},{180, 255, 255}}},
            {"green",{{ 35, 43, 46},{ 99, 255, 255}}},
            {"blue", {{100, 43, 46},{124, 255, 255}}},
            {"grey", {{ 60,  0, 46},{180,  25, 220}}}
        };

        std::pair<cv::Mat, std::vector<cv::RotatedRect>> pump_detect(cv::Mat image, int crop_x1, int crop_y1, int crop_x2, int crop_y2) {
            // Define the crop region  
            auto croppedImage = safty_cut(image, cv::Rect(crop_x1, crop_y1, crop_x2 - crop_x1, crop_y2 - crop_y1));

            int croppedImage_area = croppedImage.rows * croppedImage.cols;
            double min_area = 0.15 * croppedImage_area;
            double max_area = 0.9 * croppedImage_area;

            // 灯区域抹黑
            cv::rectangle(croppedImage, cv::Point(400, 0), cv::Point(800, 160), { 0,0,0 }, -1);

            cv::Mat hsv_img;
            cv::cvtColor(croppedImage, hsv_img, cv::COLOR_BGR2HSV);

            cv::Mat mask = cv::Mat::zeros(hsv_img.size(), CV_8UC1);

            for (auto color_list : hsv_dict) {
                auto [lowerColor, upperColor] = color_list.second;
                cv::Mat colorMask;
                cv::inRange(hsv_img, lowerColor, upperColor, colorMask);
                mask = mask | colorMask; //bitwise_or(mask, colorMask, mask);  
            }

            cv::Mat gray, new_gray, threshold, new_threshold, close, open;
            cv::cvtColor(croppedImage, gray, cv::COLOR_BGR2GRAY);
            cv::morphologyEx(gray, new_gray, cv::MORPH_BLACKHAT, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(101, 101)));
            cv::threshold(new_gray, threshold, 100, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
            cv::bitwise_and(mask, threshold, new_threshold);
            cv::morphologyEx(new_threshold, close, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15)));
            cv::morphologyEx(close, open, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15)));

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(open, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
            cv::Mat out_img = cv::Mat::zeros(open.size(), CV_8UC1);
            for (size_t i = 0; i < contours.size(); i++) {
                cv::drawContours(out_img, contours, static_cast<int>(i), cv::Scalar(255), cv::FILLED);
            }
            std::vector<cv::RotatedRect> minAreaRects;
            for (size_t i = 0; i < contours.size(); i++) {
                minAreaRects.push_back(cv::minAreaRect(contours[i]));
            }
            //std::vector<cv::RotatedRect> filteredRects;
            std::vector<cv::RotatedRect> filteredRects_reset_bl;
            for (const auto& rect : minAreaRects) {
                bool is_target = false;
                if (min_area <= rect.size.width * rect.size.height && rect.size.width * rect.size.height <= max_area) {
                    cv::Point2f newCenter = rect.center;
                    cv::Size2f newSize;
                    constexpr float SCALE = 0.7;
                    newSize.width = rect.size.width * SCALE;
                    newSize.height = rect.size.height * SCALE;
                    cv::RotatedRect rect_mul_scale{ newCenter, newSize, rect.angle };
                    cv::RotatedRect transRect = top_left_scale_quadrilateral(rect_mul_scale, 0.9, 0.6);
                    //filteredRects.push_back(rect_mul_scale);
                    filteredRects_reset_bl.push_back(transRect);
                    is_target = true;
                }
            }

            //// visualize filteredRects
            //auto draw_RotatedRects = [](std::vector<cv::RotatedRect>& rrects, cv::Mat& img, cv::Scalar color) {
            //    for (const auto& rect : rrects)
            //    {
            //        cv::Point2f pts[4];
            //        rect.points(pts);
            //        int thickness = 2;
            //        cv::line(img, pts[0], pts[1], color, thickness, cv::LINE_AA);
            //        cv::line(img, pts[1], pts[2], color, thickness, cv::LINE_AA);
            //        cv::line(img, pts[2], pts[3], color, thickness, cv::LINE_AA);
            //        cv::line(img, pts[3], pts[0], color, thickness, cv::LINE_AA);
            //    }
            //};
            //draw_RotatedRects(filteredRects, croppedImage, cv::Scalar(0, 0, 250));
            //draw_RotatedRects(filteredRects_reset_bl, croppedImage, cv::Scalar(0, 250, 0));

            return { out_img,filteredRects_reset_bl };
        };

        std::vector<cv::Point2f> RotatedRect2Polygon(const cv::RotatedRect& rrect) {
            cv::Point2f vertices[4];
            rrect.points(vertices);
            std::vector<cv::Point2f> polygon;
            for (int i = 0; i < 4; ++i) {
                polygon.push_back(vertices[i]);
            }
            return polygon;
        }

        bool isPointInsidePolygon(const cv::Point& point, const std::vector<cv::Point2f>& polygon) {
            // If the result is greater than 0, the point is inside polygon
            return cv::pointPolygonTest(polygon, point, true) > 0;
        }

        bool isPointInsideRotatedRect(const cv::Point& point, const cv::RotatedRect& rrect) {
            return isPointInsidePolygon(point, RotatedRect2Polygon(rrect));
        }

        float get_intersection_area(cv::Rect& person, cv::RotatedRect& pump) {
            cv::Point2f center(person.x + person.width / 2, person.y + person.height / 2);
            cv::Size2f size(person.width, person.height);
            float angle = 0.0f;
            cv::RotatedRect rperson(center, size, angle);

            std::vector<cv::Point2f> intersectingRegion;
            auto ic = cv::rotatedRectangleIntersection(rperson, pump, intersectingRegion);
            if (ic <= 0) {
                return 0.f;
            }
            else {
                auto area = cv::contourArea(intersectingRegion);
                return area;
            }
        }

        std::string version()
        {
            const std::string algo_module_version = "1.1.0";
            std::string nn_frame_version = "rknn2";
            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal()
    {
    }

    exposing::param_vector<pump_pumptop_person::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, const std::vector<PedestrianInfo>& pedestrian_info_list, std::map<std::string,float>& param_map_std)
    {
        return impl_->detect(bitmap, height, width, pedestrian_info_list, param_map_std);
    }

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }
}

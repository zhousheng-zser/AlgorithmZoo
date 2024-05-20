#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef BUILD_DEBUG_INFO
#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio)
#define ImgShow(visual_img) cv::imshow("visual_img", visual_img);cv::waitKey(0)
#define AdpShow(img) {auto visual_img=img.clone();ShowResize(visual_img,GetShowRatio(visual_img));ImgShow(visual_img);}
#endif // BUILD_DEBUG_INFO

namespace glasssix::pump_weld
{

    float weld_count_iou(cv::Rect rec_a, cv::Rect rec_b, bool for_a = true, bool for_b = true) {
        auto a_xmin = rec_a.x;
        auto a_ymin = rec_a.y;
        auto a_xmax = rec_a.x + rec_a.width;
        auto a_ymax = rec_a.y + rec_a.height;
        auto b_xmin = rec_b.x;
        auto b_ymin = rec_b.y;
        auto b_xmax = rec_b.x + rec_b.width;
        auto b_ymax = rec_b.y + rec_b.height;

        float x_A_and_B_min = std::max(a_xmin, b_xmin);//xmin
        float y_A_and_B_min = std::max(a_ymin, b_ymin);//ymin
        float x_A_and_B_max = std::min(a_xmax, b_xmax);//xmax
        float y_A_and_B_max = std::min(a_ymax, b_ymax);//ymax

        // intersection area. If (xmax - xmin) is negative, no intersection between A and B, set the area to 0. same to (ymax - ymin).  
        float interArea = std::max(0.0f, x_A_and_B_max - x_A_and_B_min) *
            std::max(0.0f, y_A_and_B_max - y_A_and_B_min);

        float RecA_Area = rec_a.area();
        float RecB_Area = rec_b.area();

        float iou;
        if (for_a && for_b) {
            iou = interArea / (RecA_Area + RecB_Area - interArea);
        }
        else if (for_a) {
            iou = interArea / RecA_Area;
        }
        else {
            iou = interArea / RecB_Area;
        }
        return iou;
    }

    cv::Rect change_box_of_width_and_height(const cv::Rect& box, int width, int height, int image_w, int image_h, bool add_boundary = false) {
        // Compute the center of the box  
        int x_center = box.x + box.width / 2;
        int y_center = box.y + box.height / 2;

        // Compute the new width and height based on whether we're adding a boundary  
        int new_w = add_boundary ? box.width + width : width;
        int new_h = add_boundary ? box.height + height : height;

        // Compute the new top-left and bottom-right coordinates  
        int x1 = x_center - new_w / 2;
        int y1 = y_center - new_h / 2;
        int x2 = x_center + new_w / 2;
        int y2 = y_center + new_h / 2;

        // Ensure the new box is within the image boundaries  
        x1 = std::max(0, x1);
        y1 = std::max(0, y1);
        x2 = std::min(image_w, x2);
        y2 = std::min(image_h, y2);

        // Adjust the top-left corner and width/height to fit within cv::Rect constraints  
        int new_x = x1;
        int new_y = y1;
        int new_width = x2 - x1;
        int new_height = y2 - y1;

        // Return the new box as a cv::Rect  
        return cv::Rect(new_x, new_y, new_width, new_height);
    }

    // get bounding rectangle
    cv::Rect get_outer_box(cv::Rect recA, cv::Rect recB) {
        int x_min = std::min(recA.x, recB.x);
        int y_min = std::min(recA.y, recB.y);
        int x_max = std::max(recA.x + recA.width, recB.x + recB.width);
        int y_max = std::max(recA.y + recA.height, recB.y + recB.height);

        return cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min);
    }

    void combine_related_box(std::vector<cv::Rect>& box_list, double iou_threshold = 0.5) {
        // Deletion index flags list 
        std::vector<bool> to_delete(box_list.size(), false);
        bool has_merged = false;

        do {
            has_merged = false;
            // Calculate IoU  
            for (size_t i = 0; i < box_list.size(); ++i) {
                if (to_delete[i]) continue; // Skip if this rectangle is already marked for deletion 
                for (size_t j = i + 1; j < box_list.size(); ++j) {
                    if (to_delete[j]) continue; // Skip if this rectangle is already marked for deletion  
                    if (weld_count_iou(box_list[i], box_list[j]) >= iou_threshold) {
                        // ºÏ²¢¾ØÐÎ  
                        box_list[i] = get_outer_box(box_list[i], box_list[j]);
                        to_delete[j] = true; // Mark rectangle j for deletion 
                        has_merged = true; // Indicate that a merge operation occurred  
                        break; // After merging, break out of the inner loop to check the next pair of rectangles
                    }
                }
            }
            // Delete rectangles marked for deletion  
            box_list.erase(std::remove_if(box_list.begin(), box_list.end(),
                [&to_delete, &box_list](const cv::Rect& rect) {
                    return to_delete[&rect - &box_list[0]];
                }),
                box_list.end());
            // Reset deletion flag vector  
            to_delete.resize(box_list.size(), false);
        } while (has_merged && box_list.size() > 1); // Stop the loop when no more merges occur
    }

    void get_candidate_box(std::vector<cv::Rect>& weld_box_list, int image_w, int image_h, int CANDIDATE_BOX_WIDTH, int CANDIDATE_BOX_HEIGHT) {
        constexpr float MIN_IOU_BETWEEN_CANDIDATE_BOX = 0.001f;

        for (auto& weld_box : weld_box_list) {
			weld_box = change_box_of_width_and_height(weld_box, CANDIDATE_BOX_WIDTH, CANDIDATE_BOX_HEIGHT, image_w, image_h, false);
        }

        combine_related_box(weld_box_list, MIN_IOU_BETWEEN_CANDIDATE_BOX);
    }

    //rect to xywh
    std::string RectToString(const cv::Rect& rect) {
        std::stringstream ss;
        ss << rect.x << "," << rect.y << "," << rect.width << "," << rect.height;
        return ss.str();
    }

    //xywh to rect
    cv::Rect StringToRect(const std::string& str) {
        std::vector<int> values;
        std::stringstream ss(str);
        std::string value;

        while (getline(ss, value, ',')) {
            values.push_back(std::stoi(value));
        }

        if (values.size() != 4) {
            throw glasssix::exposing::abi_not_initialized("Invalid rectangle string format. Expected 4 comma-separated integers");
        }

        return cv::Rect(values[0], values[1], values[2], values[3]);
    }

    std::vector<cv::Rect> get_weld_box(const std::vector<std::vector<cv::Rect>>& time_light_box_list) {
        constexpr float MIN_IOU_BETWEEN_LIGHT_BOX = 0.3;
        constexpr float MAX_IOU_BETWEEN_LIGHT_BOX = 0.86;
        constexpr int WELD_MIN_LIGHT_FRAME = 2;
        constexpr int WELD_MAX_LIGHT_FRAME = 3;

        std::unordered_map<std::string, std::vector<int>> box_light_dict;
        std::vector<cv::Rect> weld_box_list;

        for (size_t time_idx = 0; time_idx < time_light_box_list.size(); ++time_idx) {
            std::unordered_map<std::string, std::vector<int>> new_box_light_dict;
            std::vector<std::string> matched_box_list;

            for (const cv::Rect& light_box : time_light_box_list[time_idx]) {
                bool new_box_flag = true;

                for (const auto& pair : box_light_dict) {
                    cv::Rect existing_box = StringToRect(pair.first);
                    float iou = weld_count_iou(light_box, existing_box);
                    if (MIN_IOU_BETWEEN_LIGHT_BOX <= iou && iou <= MAX_IOU_BETWEEN_LIGHT_BOX) {
                        new_box_flag = false;
                        std::vector<int> area_list = pair.second;
                        area_list.push_back(light_box.area());
                        new_box_light_dict[RectToString(light_box)] = area_list;
                        matched_box_list.push_back(pair.first);
                        break;
                    }
                }

                if (new_box_flag) {
                    std::vector<int> area_list(time_idx, 0);
                    area_list.push_back(light_box.area());
                    new_box_light_dict[RectToString(light_box)] = area_list;
                }
            }

            // Add unlight info  
            for (const auto& pair : box_light_dict) {
                if (std::find(matched_box_list.begin(), matched_box_list.end(), pair.first) == matched_box_list.end()) {
                    std::vector<int> area_list = pair.second;
                    area_list.push_back(0);
                    new_box_light_dict[pair.first] = area_list;
                }
            }

            box_light_dict = new_box_light_dict;
        }

        for (const auto& pair : box_light_dict) {
            cv::Rect box = StringToRect(pair.first);
            const std::vector<int>& area_list = pair.second;
            int light_num = std::count_if(area_list.begin(), area_list.end(), [](int x) { return x > 0; });

            if (WELD_MIN_LIGHT_FRAME <= light_num && light_num <= WELD_MAX_LIGHT_FRAME) {
                weld_box_list.push_back(box);
            }
        }

        return weld_box_list;
    }

}

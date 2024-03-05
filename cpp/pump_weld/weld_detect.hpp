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
    void image_threshold_pipeline(const cv::Mat& image, std::vector<cv::Mat>& background_threshold_list, std::vector<cv::Mat>& light_threshold_list) {
        constexpr int LIGHT_BACKGROUND_THRESHOLD = 150;
        constexpr int LIGHT_GRAY_THRESHOLD = 235;
        cv::Mat gray, blurred, background_threshold, light_threshold;

        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
        cv::GaussianBlur(gray, blurred, cv::Size(15, 15), 0, 0);
        cv::threshold(blurred, background_threshold, LIGHT_BACKGROUND_THRESHOLD, 255, cv::THRESH_BINARY);
        cv::threshold(blurred, light_threshold, LIGHT_GRAY_THRESHOLD, 255, cv::THRESH_BINARY);

        background_threshold_list.push_back(background_threshold);
        light_threshold_list.push_back(light_threshold);
    }

    std::tuple<int, cv::Mat, std::vector<cv::Mat>> mask_image_list(const std::vector<cv::Mat>& background_threshold_list, const std::vector<cv::Mat>& light_threshold_list) {
        std::vector<int> background_threshold_sum;
        background_threshold_sum.reserve(background_threshold_list.size());

        // Compute the sum of non-zero pixels for each background threshold image  
        std::transform(background_threshold_list.begin(), background_threshold_list.end(), std::back_inserter(background_threshold_sum),
            [](const cv::Mat& img) { return cv::countNonZero(img); });

        // Find the index of the image with the minimum sum of non-zero pixels  
        auto min_it = std::min_element(background_threshold_sum.begin(), background_threshold_sum.end());
        int background_image_idx = std::distance(background_threshold_sum.begin(), min_it);

        // Get the original background mask  
        cv::Mat ori_background_mask = background_threshold_list[background_image_idx];

        // Perform morphological closing to remove small holes  
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(51, 51));
        cv::morphologyEx(ori_background_mask, ori_background_mask, cv::MORPH_CLOSE, kernel);

        // Invert the background mask  
        cv::Mat background_mask;
        cv::bitwise_not(ori_background_mask, background_mask);

        // Apply the background mask to each image in the light_threshold_list  
        std::vector<cv::Mat> masked_image_list;
        masked_image_list.reserve(light_threshold_list.size());
        std::transform(light_threshold_list.begin(), light_threshold_list.end(), std::back_inserter(masked_image_list),
            [&background_mask](const cv::Mat& img) {
                cv::Mat masked_img;
                cv::bitwise_and(img, background_mask, masked_img);
                return masked_img;
            });

        // Return the index, original background mask, and the masked image list  
        return std::make_tuple(background_image_idx, ori_background_mask, masked_image_list);
    }

    std::vector<cv::Rect> find_light_box(const cv::Mat& image) {
        constexpr int LIGHT_MIN_AERO = 30 * 30;
        constexpr int LIGHT_MAX_AERO = 200 * 200;
        constexpr int LIGHT_MAX_W_H_RATIO = 3;

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Rect> light_box_list;
        for (const auto& cnt : contours) {
            int area = cv::contourArea(cnt);
            if (LIGHT_MIN_AERO <= area && area <= LIGHT_MAX_AERO) {
				cv::Rect rect = cv::boundingRect(cnt);
                double aspectRatio = std::max(rect.width, rect.height) / std::min(rect.width, rect.height);
                if (aspectRatio <= LIGHT_MAX_W_H_RATIO) {
                    light_box_list.push_back(rect);
                }
            }
        }
        return light_box_list;
    }

    std::array<std::vector<cv::Rect>, 8> get_light_box_list(std::vector<cv::Mat>& masked_light_threshold_list) {
        std::array<std::vector<cv::Rect>, 8> time_light_box_list;
        CHECK_EQ(masked_light_threshold_list.size(), 8);
        for (int i = 0; i < 8; i++) {
            time_light_box_list[i]=find_light_box(masked_light_threshold_list[i]);
        }
        return time_light_box_list;
    }

	float count_iou(cv::Rect rec_a, cv::Rect rec_b, bool for_a = true, bool for_b = true) {
        auto a_xmin = rec_a.x;
        auto a_ymin = rec_a.y;
        auto a_xmax = rec_a.x+ rec_a.width;
        auto a_ymax = rec_a.y+ rec_a.height;
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

	int get_longest_sub_string_length(std::string& light_info, char delimiter = '0') {
        std::string longest_substring;
        std::string current_substring;

        for (char c : light_info) {
            if (c == delimiter) {
                if (current_substring.size() > longest_substring.size()) {
                    longest_substring = current_substring;
                }
                current_substring.clear(); // reset sub str
            }
            else {
                current_substring += c; // Add the char to the current substr.
            }
        }

        // Check if the last substring is the longest. 
        if (current_substring.size() > longest_substring.size()) {
            longest_substring = current_substring;
        }

        return longest_substring.size();
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
                    if (count_iou(box_list[i], box_list[j]) >= iou_threshold) {
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

    std::vector<cv::Rect> get_weld_box(const std::array<std::vector<cv::Rect>, 8>& time_light_box_list) {
		constexpr float MIN_IOU_BETWEEN_LIGHT_BOX = 0.3;
		constexpr float MAX_IOU_BETWEEN_LIGHT_BOX = 0.95;
		constexpr int WELD_MIN_LIGHT_FRAME = 4;
		constexpr int WELD_MAX_LIGHT_FRAME = 6;

        std::unordered_map<std::string, std::vector<int>> box_light_dict;
        std::vector<cv::Rect> weld_box_list;

        for (size_t time_idx = 0; time_idx < time_light_box_list.size(); ++time_idx) {
            std::unordered_map<std::string, std::vector<int>> new_box_light_dict;
            std::vector<std::string> matched_box_list;

            for (const cv::Rect& light_box : time_light_box_list[time_idx]) {
                bool new_box_flag = true;

                for (const auto& pair : box_light_dict) {
                    cv::Rect existing_box = StringToRect(pair.first);
                    float iou = count_iou(light_box, existing_box);
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
            const std::string& box_str = pair.first;
            const std::vector<int>& area_list = pair.second;

            cv::Rect box = StringToRect(box_str);

            std::vector<std::vector<int>> continue_light_area_list;
            std::vector<int> temp;
            for (int area : area_list) {
                if (area > 0) {
                    temp.push_back(area);
                }
                else {
                    if (!temp.empty()) {
                        continue_light_area_list.push_back(temp);
                        temp.clear();
                    }
                }
            }
            if (!temp.empty()) {
                continue_light_area_list.push_back(temp);
            }

            std::sort(continue_light_area_list.begin(), continue_light_area_list.end(),
                [](const std::vector<int>& a, const std::vector<int>& b) {
                return a.size() > b.size();
                });

            if (continue_light_area_list.empty()) {
                continue;
            }

            const std::vector<int>& selected_area_list = continue_light_area_list.front();
            if (selected_area_list.size() < WELD_MIN_LIGHT_FRAME || selected_area_list.size() > WELD_MAX_LIGHT_FRAME) {
                continue;
            }

            int order = 0;
            for (size_t i = 0; i < selected_area_list.size() - 1; ++i) {
                order += (selected_area_list[i + 1] > selected_area_list[i]) ? 1 : -1;
            }
            if (std::abs(order) == static_cast<int>(selected_area_list.size()) - 1) {
                continue;
            }

            weld_box_list.push_back(box);
        }

        return weld_box_list;
    }

    std::tuple<std::vector<cv::Rect>,std::vector<cv::Rect>> weld_detect(std::vector<cv::Mat>& BatchImgs, const int& height, const int& width, int candidate_box_width, int candidate_box_height) {
        std::vector<cv::Mat> background_threshold_list;
        std::vector<cv::Mat> light_threshold_list;

        for (auto image : BatchImgs) {
            image_threshold_pipeline(image, background_threshold_list, light_threshold_list);
        }

        auto [min_lt_background_idx, ori_background_mask, masked_light_threshold_list] = mask_image_list(background_threshold_list, light_threshold_list);

        std::array<std::vector<cv::Rect>, 8> time_light_box_list = get_light_box_list(masked_light_threshold_list);

        std::vector<cv::Rect> weld_box_list = get_weld_box(time_light_box_list);
        auto candidate_box_list = weld_box_list;
        get_candidate_box(candidate_box_list, width, height, candidate_box_width, candidate_box_height);

		return { weld_box_list,candidate_box_list };
    }

}

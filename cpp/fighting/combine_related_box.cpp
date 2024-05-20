#include "combine_related_box.hpp"

float count_iou_(cv::Rect rec_a, cv::Rect rec_b, bool for_a = true, bool for_b = true) {
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

// get bounding rectangle
cv::Rect get_outer_box_(const cv::Rect& recA, const cv::Rect& recB) {
    int x_min = std::min(recA.tl().x, recB.tl().x);
    int y_min = std::min(recA.tl().y, recB.tl().y);
    int x_max = std::max(recA.br().x, recB.br().x);
    int y_max = std::max(recA.br().y, recB.br().y);

    return cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min);
}

void combine_related_box(std::vector<cv::Rect>& box_list, double iou_threshold) {
    bool has_merged;
    do {
        has_merged = false;
        std::vector<bool> to_delete(box_list.size(), false);

        for (size_t i = 0; i < box_list.size(); ++i) {
            if (to_delete[i]) continue;

            for (size_t j = i + 1; j < box_list.size(); ++j) {
                if (to_delete[j]) continue;

                float iou;
                if (box_list[i].area() > box_list[j].area()) {
                    iou = count_iou_(box_list[i], box_list[j], false, true);
                }
                else {
                    iou = count_iou_(box_list[i], box_list[j], true, false);
                }

                if (iou >= iou_threshold) {
                    box_list[i] = get_outer_box_(box_list[i], box_list[j]);
                    to_delete[j] = true;
                    has_merged = true;
                    break;
                }
            }
        }

        // Erase elements marked for deletion  
        size_t index = 0;
        box_list.erase(std::remove_if(box_list.begin(), box_list.end(),
            [&to_delete, &index](const cv::Rect&) mutable -> bool {
                bool result = to_delete[index];
                ++index;
                return result;
            }), box_list.end());

    } while (has_merged && box_list.size() > 1);
}
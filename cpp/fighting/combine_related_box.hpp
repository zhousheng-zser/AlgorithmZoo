#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "opencv2/opencv.hpp"

void combine_related_box(std::vector<cv::Rect>& box_list, double iou_threshold);
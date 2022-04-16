#pragma once
#include <opencv2/opencv.hpp>

struct Detection
{
    cv::Rect box;
    float conf{};
    std::string cls{};
};


namespace utils
{
    void visualizeDetection(cv::Mat& image, std::vector<Detection>& detections);
};

#pragma once
#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>

namespace glasssix
{
namespace ring
{
//ca yolo7
struct Bbox {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int cid = 0;

    void mul_ratio(float ratio) {
        xmin = xmin * ratio;
        ymin = ymin * ratio;
        xmax = xmax * ratio;
        ymax = ymax * ratio;
    }

    std::vector<cv::Point2f> points() {
		std::vector<cv::Point2f> rect_points{
            cv::Point2f(std::round(xmin),std::round(ymin)),
            cv::Point2f(std::round(xmin),std::round(ymax)),
            cv::Point2f(std::round(xmax),std::round(ymin)),
            cv::Point2f(std::round(xmax),std::round(ymax)) };
        return rect_points;
    }

    cv::Rect get_rect() {
        return cv::Rect{
            cv::Point(std::round(xmin), std::round(ymin)),
            cv::Point(std::round(xmax), std::round(ymax)) };
    }

    cv::Point2f get_center() {
        auto p1 = cv::Point2f(std::round(xmin), std::round(ymin));
        auto p2 = cv::Point2f(std::round(xmax), std::round(ymax));
        return (p1 + p2) / 2;
    }

};


std::vector<std::array<float, 6>> concat_yolo(std::array<std::shared_ptr<glasssix::memory::tensor<float>>, 3>& yoloRst);

std::vector<Bbox> nms_cpu(std::vector<std::array<float, 6>>& target_info, float iou_thres = 0.45, float conf_thres = 0.25);

}
}

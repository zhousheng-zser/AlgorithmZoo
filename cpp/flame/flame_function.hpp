#pragma once
#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>

namespace glasssix
{
namespace flame
{
struct Bbox {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int cid = 0;

    void add(cv::Point2f point) {
        xmin += point.x;
        ymin += point.y;
        xmax += point.x;
        ymax += point.y;
    }
    void add(int x, int y) {
        xmin += x;
        ymin += y;
        xmax += x;
        ymax += y;
    }

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

    float get_area() {
        return (xmax - xmin) * (ymax - ymin);
    }
};


static inline float TensorSum(std::shared_ptr<glasssix::memory::tensor<float>> Ts) {
    double sum = 0.0;
    for (int i = 0; i < Ts->count(); ++i)
    {
        //std::cout << Ts->cpu_data()[i] << std::endl;
        sum += Ts->cpu_data()[i];
    }
    return float(sum);
}

std::vector<Bbox> concat_yolo(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& yoloRst, float conf_threshold);

void nms_cpu(std::vector<Bbox>& bboxes, float iou_thres);

}
}

#pragma once
#ifndef SOPHON_YOLOV8_WRAPPER_H
#define SOPHON_YOLOV8_WRAPPER_H

#include "yolov8.hpp"
#include "Yolov8Posture.hpp"

#include <Primitives/tensor.hpp>

using namespace std;


struct EXPORT_EXCALIBUR_PRIMITIVES Object
{
    int x1;
    int y1;
    int x2;
    int y2;
    int category;
    float score;
    std::vector<sophonkey_point> key_points;
    Object(int x1_, int y1_, int x2_, int y2_, int category_, float score_)
        : x1(x1_), y1(y1_), x2(x2_), y2(y2_), category(category_), score(score_)
    {
    }
    Object(int x1_, int y1_, int x2_, int y2_, int category_, float score_, std::vector<sophonkey_point>& key_points_) :x1(x1_), y1(y1_), x2(x2_), y2(y2_), category(category_), score(score_), key_points(key_points_)
    {}
};

class EXPORT_EXCALIBUR_PRIMITIVES SophonYolov8Wrapper
{
public: 
    SophonYolov8Wrapper(std::string model_path, int yolo_category=0, int device = 0);
    void init(float conf = 0.5, float nms_thresh = 0.6, PostureType yolov8posture = PostureType::K17);
    std::vector<Object> get_objects(cv::Mat &image,float conf=0.5,float nms_threshold =0.6);

private:
    cv::Mat padImageWidthTo64(const cv::Mat& inputImage);
    shared_ptr<BMNNContext> bm_ctx;
    BMNNHandlePtr handle;
    bm_handle_t h;
    float confidence = 0.5;
    float nmsThresh = 0.6;
    shared_ptr<YoloV8> yolov8;
    bm_image bmimg;
    YoloV8BoxVec boxes;
};


// class 

#endif // SOPHON_YOLOV8_WRAPPER_H

#pragma once
#ifndef SOPHON_YOLOV8_WRAPPER_H
#define SOPHON_YOLOV8_WRAPPER_H

#include "yolov8.hpp"

#include <Primitives/tensor.hpp>

// #ifdef _WIN32
//   #ifdef SOPHON_YOLOV8_WRAPPER_EXPORTS
//     #define SOPHON_YOLOV8_WRAPPER_API __declspec(dllexport)
//   #else
//     #define SOPHON_YOLOV8_WRAPPER_API __declspec(dllimport)
//   #endif
// #else
//   #define SOPHON_YOLOV8_WRAPPER_API
// #endif

using namespace std;

struct key_point
{
    float x;
    float y;
    float score;
    key_point(float x_, float y_, float score_) :x(x_), y(y_), score(score_)
    {}
};

struct EXPORT_EXCALIBUR_PRIMITIVES Object
{
    int x1;
    int y1;
    int x2;
    int y2;
    int category;
    float score;
    std::vector<key_point> key_points;
    Object(int x1_, int y1_, int x2_, int y2_, int category_, float score_)
        : x1(x1_), y1(y1_), x2(x2_), y2(y2_), category(category_), score(score_)
    {
    }
    Object(int x1_, int y1_, int x2_, int y2_, int category_, float score_, std::vector<key_point>& key_points_) :x1(x1_), y1(y1_), x2(x2_), y2(y2_), category(category_), score(score_), key_points(key_points_)
    {}
};

class EXPORT_EXCALIBUR_PRIMITIVES SophonYolov8Wrapper
{
public: 
    SophonYolov8Wrapper(std::string model_path, int device = 0);
    // void set_confthreshold(float conf,float nms_thresh=0.6 );
    void init(float conf = 0.5, float nms_thresh = 0.6);
    std::vector<Object> get_objects(cv::Mat &image,float conf=0.5,float nms_threshold =0.6);

private:

    shared_ptr<BMNNContext> bm_ctx;
    BMNNHandlePtr handle;
    bm_handle_t h;
    float confidence = 0.5;
    float nmsThresh = 0.6;
    shared_ptr<YoloV8> yolov8;
    bm_image bmimg;
    YoloV8BoxVec boxes;
};

#endif // SOPHON_YOLOV8_WRAPPER_H

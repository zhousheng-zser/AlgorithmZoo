#ifndef Yolov8Posture_H
#define Yolov8Posture_H

#include "yolov8.hpp"
#include <unordered_map>
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "../BMNNWrapper/bmnn_utils.h"
#include "utils.hpp"
#include "bm_wrapper.hpp"

// struct YoloV8PostureBox_keypoint
// {
//     float x;
//     float y;
//     float score;
//     YoloV8PostureBox_keypoint(float x_, float y_, float score_) :x(x_), y(y_), score(score_)
//     {}
// };

// struct YoloV8PostureBox : YoloV8Box {
//     float x1, y1, x2, y2;

//     // int x, y, width, height;
//     float score;
//     int class_id;
//     std::vector<YoloV8PostureBox_keypoint> keypoints;
// };

// using YoloV8BoxVec = std::vector<YoloV8PostureBox>;



class Yolov8Posture : public YoloV8 {
public:
    Yolov8Posture(std::shared_ptr<BMNNContext> context);

    int post_process(const bm_image& images, YoloV8BoxVec& boxes, float conf = 0.5, float nms_thresh = 0.6);
    void setyolov8_type(PostureType yolov8type);

private:
    PostureType posturetype = PostureType::K17 ;
    std::unordered_map<PostureType, int> PostureKeyinfo;
};


#endif //!Yolov8Posture_H

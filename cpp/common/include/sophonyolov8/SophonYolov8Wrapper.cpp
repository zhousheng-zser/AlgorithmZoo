#define SOPHON_YOLOV8_WRAPPER_EXPORTS
#include "SophonYolov8Wrapper.hpp"

// 实现构造函数、init方法和get_objects方法
SophonYolov8Wrapper::SophonYolov8Wrapper(std::string model_path, int device) {
    handle = make_shared<BMNNHandle>(device);
    h = handle->handle();
    bm_ctx = make_shared<BMNNContext>(handle, model_path.c_str());
    yolov8 = make_shared<YoloV8>(bm_ctx);
}

void SophonYolov8Wrapper::init(float conf, float nms_thresh) {
    yolov8->Init(conf, nms_thresh);
}


// void SophonYolov8Wrapper::set_confthreshold(float conf,float nms_thresh=0.6 )
// {
//     yolov8->set_confthreshold(conf, nms_thresh);
// }

std::vector<Object> SophonYolov8Wrapper::get_objects(cv::Mat &image,float conf,float nms_threshold) {
    cv::bmcv::toBMI(image, &bmimg, true);
    yolov8->Detect(bmimg, boxes,conf,nms_threshold );

    std::vector<Object> output;
    for (auto bbox : boxes) {
        float bboxwidth = bbox.x2 - bbox.x1;
        float bboxheight = bbox.y2 - bbox.y1;
        Object object(bbox.x1, bbox.y1, bbox.x2, bbox.y2, bbox.class_id, bbox.score);
        output.push_back(object);
    }
    
    bm_image_destroy(bmimg);
    return output;
}

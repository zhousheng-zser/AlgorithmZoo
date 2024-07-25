#define SOPHON_YOLOV8_WRAPPER_EXPORTS
#include "SophonYolov8Wrapper.hpp"

// 实现构造函数、init方法和get_objects方法
SophonYolov8Wrapper::SophonYolov8Wrapper(std::string model_path, int yolo_category, int device ){
    handle = make_shared<BMNNHandle>(device);
    h = handle->handle();
    bm_ctx = make_shared<BMNNContext>(handle, model_path.c_str());
    if(yolo_category)
        yolov8 = make_shared<Yolov8Posture>(bm_ctx);
    else
        yolov8 = make_shared<YoloV8>(bm_ctx);
}

void SophonYolov8Wrapper::init(float conf, float nms_thresh, PostureType yolov8posture) {
    yolov8->Init(conf, nms_thresh);

}

cv::Mat SophonYolov8Wrapper::padImageWidthTo64(const cv::Mat& inputImage) {
    int width = inputImage.cols;
    int height = inputImage.rows;

    int padding = 64 - (width % 64);
    if (padding == 64) {
        padding = 0; 
        return inputImage;
    }

    cv::Mat outputImage;
    cv::copyMakeBorder(inputImage, outputImage, 0, 0, 0, padding, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    return outputImage;
}

std::vector<Object> SophonYolov8Wrapper::get_objects(cv::Mat &image,float conf,float nms_threshold) {
    cv::Mat imagedevice;//默认分配soc上内存

    image.copyTo(imagedevice);

    cv::bmcv::toBMI(imagedevice, &bmimg, true);
    yolov8->Detect(bmimg, boxes,conf,nms_threshold );

    std::vector<Object> output;
    for (auto bbox : boxes) {
        float bboxwidth = bbox.x2 - bbox.x1;
        float bboxheight = bbox.y2 - bbox.y1;
        Object object(bbox.x1, bbox.y1, bbox.x2, bbox.y2, bbox.class_id, bbox.score, bbox.key_points );
        output.push_back(object);
    }
    bm_image_destroy(bmimg);
    return output;
}

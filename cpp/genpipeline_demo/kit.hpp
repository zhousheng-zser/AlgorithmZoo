#pragma once
#include <opencv2/opencv.hpp>
#include <string>

#ifdef BUILD_DEBUG_INFO
#include <opencv2/core/utils/logger.hpp>
#define SILENT_CV_LOG cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
#else
#define SILENT_CV_LOG
#endif // BUILD_DEBUG_INFO

#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio)
#define ImgShow(visual_img) cv::imshow("visual_img", visual_img);cv::waitKey(0)
#define AdpShow(img) {auto visual_img=img.clone();ShowResize(visual_img,GetShowRatio(visual_img));ImgShow(visual_img);}

struct LabConfig
{
	std::string model;
	std::string image;
	std::string postFuncStr;
	int infr_h;
	int infr_w;

	LabConfig(std::string model_, std::string image_, std::string postFuncStr_, int f_h, int f_w) :
		model(model_), image(image_), postFuncStr(postFuncStr_), infr_h(f_h), infr_w(f_w) {}
};


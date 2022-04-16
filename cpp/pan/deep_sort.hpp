#pragma once
//#include <onnxruntime_cxx_api.h>
#include "tracker.hpp"
#include "model.hpp"
#include "tracker.hpp"
#include <opencv2/opencv.hpp>
#include "dataType.hpp"

#include "utils.hpp"

//---glasssix
#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>

#include <Primitives/tensor.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Primitives/tensor_converter.hpp>
#include <Primitives/pool_allocator.hpp>

#include "hardcode.hpp"

class DeepSort {
public:
	DeepSort(std::wstring model_path, const int nn_budget = 100, const float max_cosine_distance = 0.2);
	explicit DeepSort(std::string_view param_file, std::string_view model_file, int device = -1, const int nn_budget = 100, const float max_cosine_distance = 0.2);
	explicit DeepSort(std::string_view param_file, int device = -1, const int nn_budget = 100, const float max_cosine_distance = 0.2);
	explicit DeepSort(const std::vector<std::string>& hardcode_params, std::string_view model_file, int device = -1, const int nn_budget = 100, const float max_cosine_distance = 0.2);

	std::vector<RESULT_DATA> update(cv::Mat frame, std::vector<Detection> &detect_bbox);

private:
	//装载 DETECTIONS de
	void get_detections(DETECTBOX box, float confidence, DETECTIONS& d);

	//装载所有特征 By de
	void get_all_feature(cv::Mat& frame, DETECTIONS& de);
	void get_feature(cv::Mat& frame, DETECTION_ROW& d);


	void safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi);
	cv::Mat cut_img(cv::Mat img, cv::Rect box_info);


	//CHW to NCHW , normalize, cvtColor
	std::vector<float> trans_num_suit(cv::Mat& dst, const cv::Size& output_shape, const std::vector<float>& mean_, const std::vector<float>& std_);


	//tracker
	tracker * my_tracker;


	//onnxruntime
	//Ort::Session * pipeline_;
	//Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, u8"Person_feature" };

	int device_;
	glasssix::excalibur::pipeline<float> deepsort_instance_;

	std::vector<const char*> inputNames;
	std::vector<const char*> outputNames;
};



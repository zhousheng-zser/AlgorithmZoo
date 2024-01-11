#pragma once
#ifndef _GENERAL_PIPLINE_HPP_
#define _GENERAL_PIPLINE_HPP_

#include "Excalibur/pipeline.hpp"
#include <Primitives/tensor_conversions.hpp>
#include "Primitives/logger.hpp"
#include <abi/exceptions.hpp>

#include <vector>
#include <opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>

// RKNN
#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#define USE_RKNN
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#define USE_RKNN
#endif

// ONNXRUNTIME
#ifdef USE_ONNXRT
#include "onxrt_nm.hpp"
#endif // USE_ONNXRT

using namespace glasssix;

class GenPipline {
public:
	using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

	GenPipline(std::string model, int device = -1);

	std::unordered_map<std::string, TensorSptr> forward(cv::Mat img);

#ifdef USE_ONNXRT
	void set_normalization_param(std::vector<std::array<float, 3>> normalization_param);
#endif // USE_ONNXRT

	std::string pipTypeInfo();
	int pipTypeID();

	std::string version();

private:
	enum class PipType { unknown, rknn, excalibur, onnx	};
	PipType pipType_ = PipType::unknown;

//<backends>
#ifdef USE_RKNN
	std::unique_ptr<rknnwrapper::rknn_wrapper> base_instance_rknn_;
#endif // USE_RKNN
#ifdef USE_ONNXRT
	std::unique_ptr<onx_pipline> base_instance_onnx_;
#endif // USE_ONNXRT
	std::unique_ptr<excalibur::pipeline<float>> base_instance_exbr_;
//</backends>

private:
	cv::Mat letter_image_(cv::Mat img, int hope_w, int hope_h);

};


#endif //!_GENERAL_PIPLINE_HPP_
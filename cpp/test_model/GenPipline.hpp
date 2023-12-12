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
#include "postprocessing_register.hpp"
#include "numpy_extensor/numpyExtensor.hpp"
#ifdef EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
//using namespace fs std::experimental::filesystem;
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
//using namespace std::filesystem;
namespace fs = std::filesystem;
#endif // EXPERIMENTAL_FILESYSTEM

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#define USE_RKNN
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#define USE_RKNN
#endif
#ifdef USE_ONNXRT
#include "onxrt_nm.hpp"
#endif // USE_ONNXRT

using namespace glasssix;

class GenPipline {
private:
	enum class PipType { unknown, rknn, excalibur, onnx	};
	PipType pipType_ = PipType::unknown;

#ifdef USE_RKNN
	std::unique_ptr<rknnwrapper::rknn_wrapper> base_instance_rknn_;
#endif // USE_RKNN

#ifdef USE_ONNXRT
	std::unique_ptr<onx_pipline> base_instance_onnx_;
#endif // USE_ONNXRT

	std::unique_ptr<excalibur::pipeline<float>> base_instance_exbr_;

	bool if_image_preprocess_ = false;
	bool convertBGR_ = false;
	int imgLetterReSize_ = -1;

	bool if_use_ppfunc= false;
	postprocessing_function ppfunc_;

public:

	using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

	GenPipline(std::string model, int device = -1)
	{
		if (model.size() > 5)
		{
			auto model_name = model.substr(0, model.find_last_of('.'));
			auto model_ext= model.substr(model.find_last_of('.'));
			if (model_ext == ".rknn")
			{
#ifdef USE_RKNN
				std::vector<std::string> rkn_phai;
				base_instance_rknn_ = std::make_unique<rknnwrapper::rknn_wrapper>(rkn_phai, model);
				pipType_ = PipType::rknn;
#else
				printf("System environment dnot support using rknn !");
				throw glasssix::exposing::abi_invalid_argument("Invalid model!");
#endif // USE_RKNN
			}
			else if (model_ext == ".exbr" || model_ext == ".phai")
			{
				base_instance_exbr_ = std::make_unique<excalibur::pipeline<float>>(model_name +".phai", model_name + ".racy", device);
				pipType_ = PipType::excalibur;
			}
			else if (model_ext == ".onnx")
			{
#ifdef USE_ONNXRT
				base_instance_onnx_ = std::make_unique<onx_pipline>(model_name + ".onnx");

				if (fs::exists(model_name + ".phai"))
				{
					std::cout << "[note] exists corresponding excalibur model(.phai), onnx_pip use common normalization param from " << model_name + ".phai" << std::endl;
					base_instance_onnx_->read_exbr_hardcode_params_file(model_name + ".phai");
				}

				//base_instance_onnx_->set_normalization_param({ {0,0,0},{0.0078125,0.0078125,0.0078125} });//{104,117,123},{0.0078125,0.0078125,0.0078125} 
				pipType_ = PipType::onnx;
#else
				printf("System environment dnot support using onnxruntime !");
				throw glasssix::exposing::abi_invalid_argument("Invalid model!");
#endif // USE_ONNXRT
			}
		}
	}


	std::unordered_map<std::string, TensorSptr> forward(cv::Mat image)
	{
		cv::Mat img = image.clone();
		if (if_image_preprocess_)
		{
			if (imgLetterReSize_ > 0) img = letter_image_(img, imgLetterReSize_, imgLetterReSize_);
			if (convertBGR_) cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
		}

		std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> rst_map;

		switch (pipType_)
		{
		case GenPipline::PipType::rknn:
#ifdef USE_RKNN
			rst_map = base_instance_rknn_->forward(img.data, { 1, img.rows, img.cols, img.channels() }, RKNN_TENSOR_NHWC);
#endif // USE_RKNN
			break;
		case GenPipline::PipType::excalibur:
		{
			std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
			std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
			input_tensor_u8->convert_order();
			auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>; //convenient for exporting tensor.npy file 
			rst_map = base_instance_exbr_->forward(input_tensor_f32);
		}
			break;
		case GenPipline::PipType::onnx:
#ifdef USE_ONNXRT
		{
			std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
			std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
			input_tensor_u8->convert_order();
			auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
			rst_map = base_instance_onnx_->forward(input_tensor_f32);
		}
#endif // USE_ONNXRT
			break;
		case GenPipline::PipType::unknown:
			printf("unknown model pipline type.");
			break;
		default:
			break;
		}


		if (if_use_ppfunc) {
			rst_map = ppfunc_(rst_map); // using self-define postprocessing
		}

		return rst_map;
	}

	void set_image_preprocess(int imgLetterReSize, bool convertBGR);
	void set_postprocessing(postprocessing_function ppfunc);
	void set_postprocessing(std::string ppfunc_name, std::map<std::string, postprocessing_function>& postprocessing_market);
	std::string pipTypeInfo();
	int pipTypeID();

private:

	cv::Mat letter_image_(cv::Mat img, int hope_w, int hope_h);
};


#endif //!_GENERAL_PIPLINE_HPP_
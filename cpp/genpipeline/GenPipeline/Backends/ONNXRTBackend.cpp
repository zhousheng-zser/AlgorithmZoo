#pragma once
#ifndef ONNXRTBackend_HPP  
#define ONNXRTBackend_HPP
#ifdef USE_ONNXRT

#include <memory>
#include <GenPipeline/BackendFactory.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <numeric>

#include "onxrt_pipeline.hpp"

class ONNXRTBackend : public InferBackend {
	std::unique_ptr<ONNXRTPipeline> pipeline;

public:
	static constexpr const char* backendType = "onnxruntime";
	static inline std::vector<std::string> modelFormats{ ".onnx" };

	ONNXRTBackend() = default;

	const std::string getBackendTypeName() override final {
		return backendType;
	}

	const std::string getVersion() override final {
		return pipeline->version();
	}

	int initModel(std::string arch, std::string weight, int device) override final {
		if (arch == weight) {
			auto model_pure = arch.substr(0, arch.find_last_of('.'));
			auto onx_model = model_pure + ".onnx";
			pipeline = std::make_unique<ONNXRTPipeline>(onx_model, device);
			return 0;
		}
		else {
			return 1;
		}
	}

	int initModel(std::string model, int device) override final {
		auto model_pure = model.substr(0, model.find_last_of('.'));
		auto onx_model = model_pure + ".onnx";
		pipeline = std::make_unique<ONNXRTPipeline>(onx_model, device);
		return 0;
	}

	SupportManualNormalization manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) {
		pipeline->set_normalization_param(means, stands);
		return SupportManualNormalization::Enable;
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image)  override final {
		std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, image.rows, image.cols, 3}, -1, glasssix::memory::NHWC));
#ifdef USE_BMNN
		for (int i = 0; i < image.rows; i++) {
			auto row_ptr = image.ptr<uint8_t>(i);
			std::copy(row_ptr, row_ptr + image.cols * image.channels(), input_tensor_u8->mutable_cpu_data() + i * image.cols * image.channels());
		}
#else
		std::copy(image.data, image.data + image.step[0] * image.rows, input_tensor_u8->mutable_cpu_data());
#endif
		input_tensor_u8->convert_order();
		auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
		return pipeline->forward(input_tensor_f32);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_tensor) override final {
		return pipeline->forward(input_tensor);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order)  override final {
		auto bottom = std::make_shared<glasssix::memory::tensor<float>>(data_shape);
		auto count = std::accumulate(std::begin(data_shape), std::end(data_shape), 1, std::multiplies<int>());
		std::copy(input_data, input_data + count, bottom->mutable_cpu_data());
		return pipeline->forward(bottom);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, int order)  override final {
		auto bottom_u8 = std::make_shared<glasssix::memory::tensor<std::uint8_t>>(data_shape);
		auto count = std::accumulate(std::begin(data_shape), std::end(data_shape), 1, std::multiplies<int>());
		std::copy(input_data, input_data + count, bottom_u8->mutable_cpu_data());
		auto input_tensor_f32 = bottom_u8 | glasssix::memory::tensor_convert_to<float>;
		return pipeline->forward(input_tensor_f32);
	}

};

REGISTER_BACKEND(ONNXRTBackend);


#endif // !USE_ONNXRT
#endif // !ONNXRTBackend_HPP
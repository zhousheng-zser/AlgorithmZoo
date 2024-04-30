#pragma once
#ifndef RKNNBackend_HPP  
#define RKNNBackend_HPP

#ifdef USE_RKNNAPI
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#define USE_RKNN
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#define USE_RKNN
#endif

#ifdef USE_RKNN

#include <memory>
#include <GenPipeline/BackendFactory.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <numeric>

class RKNNBackend : public InferBackend {
	std::unique_ptr<rknnwrapper::rknn_wrapper> pipeline;

public:
	static constexpr const char* backendType = "rknn";
	static inline std::vector<std::string> modelFormats{ ".rknn" };

	RKNNBackend() = default;

	const std::string getBackendTypeName() override final {
		return backendType;
	}

	const std::string getVersion() override final {
		return pipeline->version();
	}

	int initModel(std::string arch, std::string weight, int device) override final {
		if (arch == weight) {
			auto model_pure = arch.substr(0, arch.find_last_of('.'));
			auto rkn_model = model_pure + ".rknn";
			std::vector<std::string> phai;
			pipeline = std::make_unique<rknnwrapper::rknn_wrapper>(phai, rkn_model, device);
			return 0;
		}
		else {
			return 1;
		}
	}

	// invalid phai
	int initModel(const std::vector<std::string>& phai, std::string weight, int device) override final {
		auto model_pure = weight.substr(0, weight.find_last_of('.'));
		auto rkn_model = model_pure + ".rknn";
		std::vector<std::string> phai;
		pipeline = std::make_unique<rknnwrapper::rknn_wrapper>(phai, rkn_model, device);
		return 0;
	}

	int initModel(std::string model, int device) override final {
		auto model_pure = model.substr(0, model.find_last_of('.'));
		auto rkn_model = model_pure + ".rknn";
		std::vector<std::string> phai;
		pipeline = std::make_unique<rknnwrapper::rknn_wrapper>(phai, rkn_model, device);
		return 0;
	}

	SupportManualNormalization manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) {
		// rknn useless normalization param setting functiuon, model file integrated;
		return SupportManualNormalization::Disable;
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image)  override final {
		return pipeline->forward(image.data, { 1, image.rows, image.cols, image.channels() }, RKNN_TENSOR_NHWC);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_tensor) override final {
		return pipeline->forward(input_tensor->cpu_data(), input_tensor->data_shape(), static_cast<rknn_tensor_format>(input_tensor->order()));
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order)  override final {
		return pipeline->forward(input_data, data_shape, static_cast<rknn_tensor_format>(order));//RKNN_TENSOR_NCHW
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, int order)  override final {
		return pipeline->forward(input_data, data_shape, static_cast<rknn_tensor_format>(order));//RKNN_TENSOR_NCHW
	}

};

REGISTER_BACKEND(RKNNBackend);


#endif // !USE_RKNN

#endif // !RKNNBackend_HPP
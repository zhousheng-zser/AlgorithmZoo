#pragma once
#ifndef ExcaliburBackend_HPP  
#define ExcaliburBackend_HPP
#include <memory>
#include <GenPipeline/BackendFactory.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <numeric>

#include <Excalibur/pipeline.hpp>

class ExcaliburBackend : public InferBackend {
    std::unique_ptr<glasssix::excalibur::pipeline<float>> pipeline;

public:
    static constexpr const char* backendType = "excalibur";
    static inline std::vector<std::string> modelFormats{ ".phai",".racy",".exbr" };

	ExcaliburBackend() = default;

	const std::string getBackendTypeName() override final {
		return backendType;
	}

	const std::string getVersion() override final {
		return pipeline->version();
	}

    int initModel(std::string arch, std::string weight, int device) override final {
        pipeline = std::make_unique<glasssix::excalibur::pipeline<float>>(arch, weight, device);
        return 0;
    }

    int initModel(std::string model, int device) override final {
        auto model_pure = model.substr(0, model.find_last_of('.'));
        auto phai = model_pure + ".phai";
        auto racy = model_pure + ".racy";
        pipeline = std::make_unique<glasssix::excalibur::pipeline<float>>(phai, racy, device);
        return 0;
    }

    SupportManualNormalization manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) {
        // excalibur useless normalization param setting functiuon, model file integrated;
        return SupportManualNormalization::Disable;
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

REGISTER_BACKEND(ExcaliburBackend);


#endif // ExcaliburBackend_HPP
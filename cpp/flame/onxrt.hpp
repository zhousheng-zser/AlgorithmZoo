#pragma once
#ifndef _ONNXRT_HPP_
#define _ONNXRT_HPP_

#include <vector>
#include <string>
#include <unordered_map>
#include <onnxruntime_cxx_api.h>
#include <onnxruntime_c_api.h>
#include "Primitives/tensor.hpp"
#include "Primitives/fmt/format.h"

namespace onnxrt
{
	class pipline {
		// frame
		Ort::Env env_;
		Ort::Session* session_ptr_ = nullptr;
		Ort::SessionOptions session_options_;
		Ort::AllocatorWithDefaultOptions allocator_;
	public:

		size_t inNodes_Num_;
		size_t outNodes_Num_;

		std::vector<std::string> model_inNode_names_;
		std::vector<std::string> model_outNode_names_;

		std::vector<std::vector<int64_t>> model_input_shape_;
		std::vector<std::vector<int64_t>> model_output_shape_;

		std::string version() {
			return "onnxruntime";
		}

		pipline(std::string model_path)
		{
			OrtCUDAProviderOptions options;
			options.device_id = 0;
			options.arena_extend_strategy = 0;
			//options.gpu_mem_limit = (size_t)1 * 1024 * 1024 * 1024; //onnxruntime1.8.1, onnxruntime1.9.0
			options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearch::OrtCudnnConvAlgoSearchHeuristic;
			options.do_copy_in_default_stream = 1;
			session_options_.AppendExecutionProvider_CUDA(options);

			session_options_.SetIntraOpNumThreads(4);
			session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL); //ORT_ENABLE_ALL ORT_ENABLE_EXTENDED ORT_ENABLE_BASIC
			//OrtSessionOptionsAppendExecutionProvider_CUDA(session_options_, 0);

			env_ = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "TheNet");
			std::wstring widestr = std::wstring(model_path.begin(), model_path.end());
			session_ptr_ = new Ort::Session(env_, widestr.data(), session_options_);

			//Ort::SessionOptions::AppendExecutionProvider_CUDA(*session_ptr_,0);

			inNodes_Num_ = session_ptr_->GetInputCount();
			outNodes_Num_ = session_ptr_->GetOutputCount();

			for (int i = 0; i < inNodes_Num_; i++)
			{
				auto InputNameAlc = session_ptr_->GetInputNameAllocated(i, allocator_);
				model_inNode_names_.push_back(InputNameAlc.get());
				model_input_shape_.push_back((*session_ptr_).GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
			}

			for (int i = 0; i < outNodes_Num_; i++)
			{
				auto OutputNameAlc = session_ptr_->GetOutputNameAllocated(i, allocator_);
				model_outNode_names_.push_back(OutputNameAlc.get());
				model_output_shape_.push_back((*session_ptr_).GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
			}
		}

		~pipline()
		{
			if (session_ptr_ != nullptr) {
				delete session_ptr_;
				session_ptr_ = nullptr;
			}
		}

		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_exbtensor) {
			auto exbTensorShape = input_exbtensor->data_shape();

			std::vector<int64_t> input_Img_shape(exbTensorShape.begin(), exbTensorShape.end());
			Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

			Ort::Value input_onxTensor = Ort::Value::CreateTensor<float>(
				memory_info,
				input_exbtensor->mutable_cpu_data(),
				input_exbtensor->count(),
				input_Img_shape.data(),
				input_Img_shape.size()
				);

			std::vector<const char*> model_inNode_names;
			std::vector<const char*> model_outNode_names;
			for (auto& inName : model_inNode_names_) {
				model_inNode_names.push_back(inName.c_str());
			}
			for (auto& ouName : model_outNode_names_) {
				model_outNode_names.push_back(ouName.c_str());
			}

			std::vector<Ort::Value> output_tensors = (*session_ptr_).Run(
				Ort::RunOptions{ nullptr }, // run_options
				model_inNode_names.data(),
				&input_onxTensor,
				model_inNode_names_.size(),
				model_outNode_names.data(),
				model_outNode_names_.size()
			);

			std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> result_map;
			for (int i = 0; i < outNodes_Num_; i++)
			{
				auto shape = output_tensors[i].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<int> ts_shape(shape.begin(), shape.end());
				std::shared_ptr<glasssix::memory::tensor<float>> output_exbtensor;
				output_exbtensor.reset(new glasssix::memory::tensor<float>(ts_shape, -1, glasssix::memory::orderType::NCHW));
				auto data_ptr = output_tensors[i].GetTensorMutableData<float>();
				std::memcpy(output_exbtensor->mutable_cpu_data(), data_ptr, sizeof(float) * output_exbtensor->count());

				model_outNode_names_[i];
				result_map.try_emplace(model_outNode_names_[i], output_exbtensor);
			}

			return result_map;
		}
	};
}

#endif //!_ONNXRT_HPP_
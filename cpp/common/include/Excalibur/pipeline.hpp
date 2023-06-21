#pragma once
#ifndef _PIPELINE_HPP_
#define _PIPELINE_HPP_

#include "Primitives/tensor.hpp"
#include "Primitives/dllexport.hpp"

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class EXPORT_EXCALIBUR_PRIMITIVES pipeline
		{
		public:
			class impl;

			explicit pipeline();
			explicit pipeline(std::string_view param_file, std::string_view model_file, int device = -1);
			explicit pipeline(std::string_view param_file, int device = -1);
			explicit pipeline(const std::vector<std::string>& hardcode_params, std::string_view model_file, int device = -1);
			~pipeline();

			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward_cpu(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward_gpu(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::shared_ptr<memory::tensor<Dtype>> get_featmap(std::string_view featmap_name);

#ifdef SUPPORT_QUANTIZATION
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>>  get_blob_ptr();
#endif

			void enable_profiler();
			void disable_profiler();

			static std::string version();
		private:
			impl* impl_;
			DISABLE_COPY_AND_ASSIGN(pipeline);
		};
	}
}
#endif // !_PIPELINE_HPP_

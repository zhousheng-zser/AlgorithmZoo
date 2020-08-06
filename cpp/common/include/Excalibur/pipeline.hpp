#pragma once
#ifndef _PIPELINE_HPP_
#define _PIPELINE_HPP_

#include "Primitives/tensor.hpp"

#include <string>
#include <vector>
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
			explicit pipeline(std::string param_file, std::string model_file, int device = -1);
			explicit pipeline(std::string param_file, int device = -1);
			explicit pipeline(std::vector<std::string> hardcode_params, std::string model_file, int device = -1);
			~pipeline();

			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward_cpu(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>> forward_gpu(const std::shared_ptr<memory::tensor<Dtype>>& input_tensor);
			std::shared_ptr<memory::tensor<Dtype>> get_featmap(std::string featmap_name);
			void enable_profiler();
			void disable_profiler();
		private:
			impl* impl_;
			DISABLE_COPY_AND_ASSIGN(pipeline);
		};
	}
}
#endif // !_PIPELINE_HPP_

#pragma once
#ifndef _OPERATION_RELU_HPP_
#define _OPERATION_RELU_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_relu : public operation<Dtype>
		{
		public:
			explicit operation_relu(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_relu() {}

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

			virtual void forward_gpu_f32(
#ifdef USE_CUDA
				cublasHandle_t &cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			float slope_ = 0.0f;
		};
	}
}
#endif // !_OPERATION_RELU_HPP_


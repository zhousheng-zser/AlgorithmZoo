#pragma once
#ifndef _OPERATION_SIGMOID_HPP_
#define _OPERATION_SIGMOID_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_sigmoid : public operation<Dtype>
		{
		public:
			explicit operation_sigmoid(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_sigmoid() {}

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
			int axis_ = 0;
			/// sum_multiplier is used to carry out sum using BLAS
			std::shared_ptr<memory::tensor<float>> sum_multiplier_;
			/// scale is an intermediate Blob to hold temporary results.
			std::shared_ptr<memory::tensor<float>> scale_;
		};
	}
}
#endif // !_OPERATION_SIGMOID_HPP_


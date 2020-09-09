#pragma once
#ifndef _OPERATION_RESHAPE_HPP_
#define _OPERATION_RESHAPE_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_reshape : public operation<Dtype>
		{
		public:
			explicit operation_reshape(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_reshape() {}


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
			int n_ = 0;
			int c_ = -233;
			int h_ = -233;
			int w_ = -233;
			int permute_ = 0;
		};
	}
}
#endif // !_OPERATION_RESHAPE_HPP_


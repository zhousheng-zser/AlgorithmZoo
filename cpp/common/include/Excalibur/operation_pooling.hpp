#pragma once
#ifndef _OPERATION_POOLING_HPP_
#define _OPERATION_POOLING_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_pooling : public operation<Dtype>
		{
		public:
			explicit operation_pooling(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_pooling() {}


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
			int kernel_size_w_ = 0;
			int kernel_size_h_ = 0;
			int stride_w_ = 1;
			int stride_h_ = 1;
			int pad_left_ = 0;
			int pad_right_ = 0;
			int pad_top_ = 0;
			int pad_bottom_ = 0;
			enum pooling_type { MAX, AVE };
			pooling_type type_;
			bool global_pooling_;
			int pad_mode_;
		};
	}
}
#endif // !_OPERATION_POOLING_HPP_


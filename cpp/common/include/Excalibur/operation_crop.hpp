#pragma once
#ifndef _OPERATION_CROP_HPP_
#define _OPERATION_CROP_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_crop : public operation<Dtype>
		{
		public:
			explicit operation_crop(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_crop() {}

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
			int woffset_ = 0;
			int hoffset_ = 0;
			int coffset_ = 0;
			int outw_ = 0;
			int outh_ = 0;
			int outc_ = 0;
			int woffset2_ = 0;
			int hoffset2_ = 0;
			int coffset2_ = 0;
			std::vector<int> starts_;
			std::vector<int> ends_;
			std::vector<int> axis_;
		};
	}
}
#endif // !_OPERATION_CROP_HPP_

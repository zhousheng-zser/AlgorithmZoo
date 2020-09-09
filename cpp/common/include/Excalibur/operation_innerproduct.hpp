#pragma once
#ifndef _OPERATION_INNERPRODUCT_HPP_
#define _OPERATION_INNERPRODUCT_HPP_
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_innerproduct : public operation<Dtype>
		{
		public:
			explicit operation_innerproduct(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_innerproduct() {}

			virtual int init_weights();

			virtual int init_weights(FILE* fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

			virtual void forward_gpu_f32(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			int num_output_ = 0;
			bool bias_term_ = false;
			int weight_data_size_ = 0;
			bool int8_scale_term_ = false;
			int activation_type_ = 0;
			std::string activation_params_;

			std::shared_ptr<memory::tensor<float>> bias_multiplier_;
		};
	}
}
#endif // !_OPERATION_INNERPRODUCT_HPP_


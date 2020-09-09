#ifndef _OPERATION_DECONVOLUTIONDEPTHWISE_HPP_
#define _OPERATION_DECONVOLUTIONDEPTHWISE_HPP_
#include "operation_deconvlution.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_deconvolutiondepthwise : public operation_deconvolution<Dtype>
		{
		public:
			operation_deconvolutiondepthwise(const operation_param& param);

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
		};
	}
}
#endif // !_OPERATION_DECONVOLUTIONDEPTHWISE_HPP_

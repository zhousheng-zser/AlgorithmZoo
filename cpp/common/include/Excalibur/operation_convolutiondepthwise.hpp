#ifndef _OPERATION_CONVOLUTIONDEPTHWISE_HPP_
#define _OPERATION_CONVOLUTIONDEPTHWISE_HPP_
#include "operation_convolution.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_convolutiondepthwise : public operation_convolution<Dtype>
		{
		public:
			operation_convolutiondepthwise(const operation_param& param);

#ifdef HARDCODE
			virtual void init_weights() {}
#else
			virtual int init_weights(FILE *fp);
#endif //!HARDCODE

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

			virtual void forward_cpu_i8(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
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
			std::shared_ptr<memory::tensor<float>> U_;
			std::shared_ptr<memory::tensor<float>> V_;
			std::shared_ptr<memory::tensor<float>> kernel_tm_;
			std::shared_ptr<memory::tensor<float>> border_bottom_;


			void forward_winograd_f32(std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);

			void forward_k3s1_f32(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);

			void forward_k3s2_f32(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
		
			int dequantize_int8(const std::shared_ptr<memory::tensor<signed char>>& src,
				std::shared_ptr<memory::tensor<float>>& dst, std::vector<float> scale);
		};
	}
}
#endif // !_OPERATION_CONVOLUTIONDEPTHWISE_HPP_

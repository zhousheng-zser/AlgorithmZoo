#pragma once
#ifndef _OPERATION_CONVOLUTION_HPP_
#define _OPERATION_CONVOLUTION_HPP_

#include "operation_general_conv.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_convolution : public operation_general_conv<Dtype>
		{
		public:
			operation_convolution(const operation_param& param);
			virtual ~operation_convolution() {}
			virtual int init_weights();

			virtual int init_weights(FILE *fp);

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

			void conv3x3s1_winograd23_tr_kernel();

			void conv3x3s1_winograd23(const std::shared_ptr<memory::tensor<float> >& bottom_blob, 
				std::shared_ptr<memory::tensor<float> >& top_blob);

			void forward_im2col_tr_kernel();

			void forward_im2col(const std::shared_ptr<memory::tensor<float> >& bottom_blob, 
				std::shared_ptr<memory::tensor<float> >& top_blob);

			void conv3x3s1_winograd23_tr_kernel_int8();

			void conv3x3s1_winograd23_int8(const std::shared_ptr<memory::tensor<signed char> >& bottom_blob, 
				std::shared_ptr<memory::tensor<int> >& top_blob);

			void forward_im2col_int8_tr_kernel();

			void conv_im2col_sgemm_int8_dequant_sse(const std::shared_ptr<memory::tensor<signed char> >& bottom_blob, 
				std::shared_ptr<memory::tensor<float> >& top_blob, float scale_dequant);


			//others
			void forward_cpu_sgemm(const float* input, const float* weights, float* output, memory::orderType order);

			void forward_cpu_sbias(float* output, const float* bias, memory::orderType order);

#ifdef USE_CUDA
			void forward_gpu_sgemm(cublasHandle_t &cublas_handle, const float* input, const float* weights, float* output, memory::orderType order);

			void forward_gpu_sbias(cublasHandle_t &cublas_handle, float* output, const float* bias, memory::orderType order);
#endif //!USE_CUDA

			void forward_cpu_k1s1_f32(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);

			void quantize_float32_to_int8(const std::shared_ptr<memory::tensor<float>>& src,
				std::shared_ptr<memory::tensor<signed char>>& dst, float scale);

			void dequantize_int32_to_float32(std::shared_ptr<memory::tensor<int>>& src,
				std::shared_ptr<memory::tensor<float>>& dst, float scale);


			//f32 convolution multiplication
			std::shared_ptr<memory::tensor<float>> weights1x1_;
			std::shared_ptr<memory::tensor<float>> col_buffer_;
			std::shared_ptr<memory::tensor<float>> bias_multiplier_;
			std::vector<std::shared_ptr<memory::tensor<float>>> kernel_tm_;
			std::vector<std::shared_ptr<memory::tensor<float>>> kernel_tm_gemm_;
			std::shared_ptr<memory::tensor<float>> tmp_;
			std::shared_ptr<memory::tensor<float>> bottom_im2col_;
			std::shared_ptr<memory::tensor<float>> bottom_tm_;
			std::shared_ptr<memory::tensor<float>> border_bottom_;
			std::shared_ptr<memory::tensor<float> > top_blob_bordered_;
			std::shared_ptr<memory::tensor<float> > bottom_blob_bordered_;
			std::shared_ptr<memory::tensor<float> > bottom_blob_tm_;


			//int8 convolution multiplication
			std::shared_ptr<memory::tensor<int>> top_int32_;            
			std::shared_ptr<memory::tensor<signed char>> bottom_int8_;
			std::shared_ptr<memory::tensor<signed char>> bottom_int8_bordered_;
			std::vector<std::shared_ptr<memory::tensor<signed char>>> kernel_tm_int8_sgemm_;
			std::vector<std::shared_ptr<memory::tensor<short>>> kernel_tm_int8_;
			std::shared_ptr<memory::tensor<signed char>> weights1x1_int8_;
			std::shared_ptr<memory::tensor<signed char>> col_buffer_int8_;
			std::shared_ptr<memory::tensor<short> > bottom_blob_int8_tm_;
			std::shared_ptr<memory::tensor<signed char>> bottom_im2col_int8_;
			std::shared_ptr<memory::tensor<signed char>> bottom_tm_int8_;
			std::shared_ptr<memory::tensor<signed char>> bottom_im2row_;
			


			float* col_buffer_data;
			float* bias_multiplier_data;
		};
	}
}
#endif // !_OPERATION_CONVOLUTION_HPP_

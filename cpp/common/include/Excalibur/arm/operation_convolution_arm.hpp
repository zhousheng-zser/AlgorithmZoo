#pragma once
#ifndef _OPERATION_CONVOLUTION_ARM_HPP_
#define _OPERATION_CONVOLUTION_ARM_HPP_

#include "../operation_general_conv.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_convolution_arm : public operation_general_conv<Dtype>
		{
		public:
			operation_convolution_arm(const operation_param& param);

			virtual int init_weights();

			virtual int init_weights(FILE *fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:

			void conv1x1s1_sgemm_transform_kernel_neon();
			void conv3x3s1_winograd64_transform_kernel_neon5();
			void conv3x3s2_transform_kernel_neon();
			void conv_im2col_sgemm_transform_kernel_neon();

			void conv1x1s1_sgemm_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void conv1x1s1_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void conv3x3s1_winograd64_neon5(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void conv3x3s1_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void conv3x3s2_packed_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void conv_im2col_sgemm_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);

			//f32 convolution multiplication
			std::shared_ptr<memory::tensor<float>> kernel_tm_;
			std::shared_ptr<memory::tensor<float>> kernel_tm_gemm_;
		};
	}
}
#endif // !_OPERATION_CONVOLUTION_ARM_HPP_

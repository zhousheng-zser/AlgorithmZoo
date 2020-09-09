#pragma once
#ifndef _OPERATION_CONVOLUTION_ARM_HPP_
#define _OPERATION_CONVOLUTION_ARM_HPP_

#include "../operation_convolutiondepthwise.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_convolutiondepthwise_arm : public operation_convolutiondepthwise<Dtype>
		{
		public:
			operation_convolutiondepthwise_arm(const operation_param& param);

			virtual int init_weights();

			virtual int init_weights(FILE *fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			void convdw3x3s1_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
			void convdw3x3s2_neon(const std::shared_ptr < memory::tensor<float>>& bottom,
				std::shared_ptr < memory::tensor<float>>& top);
		};
	}
}
#endif // !_OPERATION_CONVOLUTION_ARM_HPP_

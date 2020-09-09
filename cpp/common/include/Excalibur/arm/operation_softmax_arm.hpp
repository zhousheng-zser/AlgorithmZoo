#pragma once
#ifndef _OPERATION_SOFTMAX_ARM_HPP_
#define _OPERATION_SOFTMAX_ARM_HPP_
#include "../operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_softmax_arm : public operation<Dtype>
		{
		public:
			explicit operation_softmax_arm(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_softmax_arm() {}

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
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
#endif // !_OPERATION_SOFTMAX_HPP_


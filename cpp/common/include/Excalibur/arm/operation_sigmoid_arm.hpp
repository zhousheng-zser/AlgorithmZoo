#pragma once
#ifndef _OPERATION_SIGMOID_ARM_HPP_
#define _OPERATION_SIGMOID_ARM_HPP_
#include "../operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_sigmoid_arm : public operation<Dtype>
		{
		public:
			explicit operation_sigmoid_arm(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_sigmoid_arm() {}

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);
		};
	}
}
#endif // !_OPERATION_SIGMOID_HPP_


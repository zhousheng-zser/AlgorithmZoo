#pragma once
#ifndef _OPERATION_PRELU_ARM_HPP_
#define _OPERATION_PRELU_ARM_HPP_
#include "../operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_prelu_arm : public operation<Dtype>
		{
		public:
			explicit operation_prelu_arm(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_prelu_arm() {}

			virtual int init_weights();

			virtual int init_weights(FILE *fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			int num_slope_ = 0;
			bool share_channel_ = false;
		};
	}
}
#endif // !_OPERATION_PRELU_HPP_


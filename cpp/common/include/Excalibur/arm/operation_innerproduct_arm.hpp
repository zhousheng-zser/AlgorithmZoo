#pragma once
#ifndef _OPERATION_INNERPRODUCT_ARM_HPP_
#define _OPERATION_INNERPRODUCT_ARM_HPP_
#include "../operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_innerproduct_arm : public operation<Dtype>
		{
		public:
			explicit operation_innerproduct_arm(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_innerproduct_arm() {}

			virtual int init_weights();

			virtual int init_weights(FILE *fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			int num_output_ = 0;
			bool bias_term_ = false;
			int weight_data_size_ = 0;
			bool int8_scale_term_ = false;
			int activation_type_ = 0;
			std::string activation_params_;
		};
	}
}
#endif // !_OPERATION_INNERPRODUCT_HPP_


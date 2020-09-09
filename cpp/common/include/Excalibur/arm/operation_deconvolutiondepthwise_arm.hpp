#ifndef _OPERATION_DECONVOLUTIONDEPTHWISE_ARM_HPP_
#define _OPERATION_DECONVOLUTIONDEPTHWISE_ARM_HPP_
#include "../operation_general_conv.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_deconvolutiondepthwise_arm : public operation_general_conv<Dtype>
		{
		public:
			operation_deconvolutiondepthwise_arm(const operation_param& param);

			virtual int init_weights();

			virtual int init_weights(FILE *fp);

		protected:
			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops);

		private:
			std::shared_ptr<memory::tensor<float>> kernel_pack1_;
		};
	}
}
#endif // !_OPERATION_DECONVOLUTIONDEPTHWISE_HPP_

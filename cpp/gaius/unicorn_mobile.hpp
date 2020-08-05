#ifndef _UNICORN_MOBILE_HPP_
#define _UNICORN_MOBILE_HPP_

#include "Primitives/tensor.hpp"
#include "Excalibur/support_layers.hpp"
#include "Excalibur/tensor_operation_cpu.hpp"
#include "Excalibur/tensor_operation_gpu.hpp"

using namespace glasssix::excalibur;

namespace glasssix
{
	namespace gaius
	{
		class Unicorn_mobile
		{
			Declear_Params(conv1);
			Declear_Params(relu1);
			Declear_Params(conv1_dw);
			Declear_Params(relu1_dw);
			Declear_Params(conv2_ex);
			Declear_Params(relu2_ex);
			Declear_Params(conv2_dw);
			Declear_Params(relu2_dw);
			Declear_Params(conv2_em);
			Declear_Params(conv2_1_ex);
			Declear_Params(relu2_1_ex);
			Declear_Params(conv2_1_dw);
			Declear_Params(relu2_1_dw);
			Declear_Params(conv2_1_em);
			Declear_Params(conv2_2_ex);
			Declear_Params(relu2_2_ex);
			Declear_Params(conv2_2_dw);
			Declear_Params(relu2_2_dw);
			Declear_Params(conv2_2_em);
			Declear_Params(conv2_3_ex);
			Declear_Params(relu2_3_ex);
			Declear_Params(conv2_3_dw);
			Declear_Params(relu2_3_dw);
			Declear_Params(conv2_3_em);
			Declear_Params(conv2_4_ex);
			Declear_Params(relu2_4_ex);
			Declear_Params(conv2_4_dw);
			Declear_Params(relu2_4_dw);
			Declear_Params(conv2_4_em);
			Declear_Params(conv3_ex);
			Declear_Params(relu3_ex);
			Declear_Params(conv3_dw);
			Declear_Params(relu3_dw);
			Declear_Params(conv3_em);
			Declear_Params(conv3_1_ex);
			Declear_Params(relu3_1_ex);
			Declear_Params(conv3_1_dw);
			Declear_Params(relu3_1_dw);
			Declear_Params(conv3_1_em);
			Declear_Params(conv3_2_ex);
			Declear_Params(relu3_2_ex);
			Declear_Params(conv3_2_dw);
			Declear_Params(relu3_2_dw);
			Declear_Params(conv3_2_em);
			Declear_Params(conv3_3_ex);
			Declear_Params(relu3_3_ex);
			Declear_Params(conv3_3_dw);
			Declear_Params(relu3_3_dw);
			Declear_Params(conv3_3_em);
			Declear_Params(conv3_4_ex);
			Declear_Params(relu3_4_ex);
			Declear_Params(conv3_4_dw);
			Declear_Params(relu3_4_dw);
			Declear_Params(conv3_4_em);
			Declear_Params(conv3_5_ex);
			Declear_Params(relu3_5_ex);
			Declear_Params(conv3_5_dw);
			Declear_Params(relu3_5_dw);
			Declear_Params(conv3_5_em);
			Declear_Params(conv3_6_ex);
			Declear_Params(relu3_6_ex);
			Declear_Params(conv3_6_dw);
			Declear_Params(relu3_6_dw);
			Declear_Params(conv3_6_em);
			Declear_Params(conv4_ex);
			Declear_Params(relu4_ex);
			Declear_Params(conv4_dw);
			Declear_Params(relu4_dw);
			Declear_Params(conv4_em);
			Declear_Params(conv4_1_ex);
			Declear_Params(relu4_1_ex);
			Declear_Params(conv4_1_dw);
			Declear_Params(relu4_1_dw);
			Declear_Params(conv4_1_em);
			Declear_Params(conv4_2_ex);
			Declear_Params(relu4_2_ex);
			Declear_Params(conv4_2_dw);
			Declear_Params(relu4_2_dw);
			Declear_Params(conv4_2_em);
			Declear_Params(conv5_ex);
			Declear_Params(relu5_ex);
			Declear_Params(conv5_dw);
			Declear_Params(fc5);

			//
			int device_;
			bool cudnn_ready_ = false;
			bool int8_quantization_ = false;

			std::shared_ptr<memory::tensor<unsigned char>> tensor_unsigned_char_data = nullptr;
			std::shared_ptr<memory::tensor<float>> tensor_float_data = nullptr;
			std::vector<float> quality_score;
			//

			Declear_Opration(baseconv, conv1);
			Neuron_Name(conv1);
			Declear_Opration(prelu, relu1);
			Neuron_Name(relu1);
			Declear_Opration(baseconv, conv1_dw);
			Neuron_Name(conv1_dw);
			Declear_Opration(prelu, relu1_dw);
			Neuron_Name(relu1_dw);
			Declear_Opration(baseconv, conv2_ex);
			Neuron_Name(conv2_ex);
			Declear_Opration(prelu, relu2_ex);
			Neuron_Name(relu2_ex);
			Declear_Opration(baseconv, conv2_dw);
			Neuron_Name(conv2_dw);
			Declear_Opration(prelu, relu2_dw);
			Neuron_Name(relu2_dw);
			Declear_Opration(baseconv, conv2_em);
			Neuron_Name(conv2_em);
			Declear_Opration(baseconv, conv2_1_ex);
			Neuron_Name(conv2_1_ex);
			Declear_Opration(prelu, relu2_1_ex);
			Neuron_Name(relu2_1_ex);
			Declear_Opration(baseconv, conv2_1_dw);
			Neuron_Name(conv2_1_dw);
			Declear_Opration(prelu, relu2_1_dw);
			Neuron_Name(relu2_1_dw);
			Declear_Opration(baseconv, conv2_1_em);
			Neuron_Name(conv2_1_em);
			Declear_Opration(eltwise, res2_1);
			Neuron_Name(res2_1);
			Declear_Opration(baseconv, conv2_2_ex);
			Neuron_Name(conv2_2_ex);
			Declear_Opration(prelu, relu2_2_ex);
			Neuron_Name(relu2_2_ex);
			Declear_Opration(baseconv, conv2_2_dw);
			Neuron_Name(conv2_2_dw);
			Declear_Opration(prelu, relu2_2_dw);
			Neuron_Name(relu2_2_dw);
			Declear_Opration(baseconv, conv2_2_em);
			Neuron_Name(conv2_2_em);
			Declear_Opration(eltwise, res2_2);
			Neuron_Name(res2_2);
			Declear_Opration(baseconv, conv2_3_ex);
			Neuron_Name(conv2_3_ex);
			Declear_Opration(prelu, relu2_3_ex);
			Neuron_Name(relu2_3_ex);
			Declear_Opration(baseconv, conv2_3_dw);
			Neuron_Name(conv2_3_dw);
			Declear_Opration(prelu, relu2_3_dw);
			Neuron_Name(relu2_3_dw);
			Declear_Opration(baseconv, conv2_3_em);
			Neuron_Name(conv2_3_em);
			Declear_Opration(eltwise, res2_3);
			Neuron_Name(res2_3);
			Declear_Opration(baseconv, conv2_4_ex);
			Neuron_Name(conv2_4_ex);
			Declear_Opration(prelu, relu2_4_ex);
			Neuron_Name(relu2_4_ex);
			Declear_Opration(baseconv, conv2_4_dw);
			Neuron_Name(conv2_4_dw);
			Declear_Opration(prelu, relu2_4_dw);
			Neuron_Name(relu2_4_dw);
			Declear_Opration(baseconv, conv2_4_em);
			Neuron_Name(conv2_4_em);
			Declear_Opration(eltwise, res2_4);
			Neuron_Name(res2_4);
			Declear_Opration(baseconv, conv3_ex);
			Neuron_Name(conv3_ex);
			Declear_Opration(prelu, relu3_ex);
			Neuron_Name(relu3_ex);
			Declear_Opration(baseconv, conv3_dw);
			Neuron_Name(conv3_dw);
			Declear_Opration(prelu, relu3_dw);
			Neuron_Name(relu3_dw);
			Declear_Opration(baseconv, conv3_em);
			Neuron_Name(conv3_em);
			Declear_Opration(baseconv, conv3_1_ex);
			Neuron_Name(conv3_1_ex);
			Declear_Opration(prelu, relu3_1_ex);
			Neuron_Name(relu3_1_ex);
			Declear_Opration(baseconv, conv3_1_dw);
			Neuron_Name(conv3_1_dw);
			Declear_Opration(prelu, relu3_1_dw);
			Neuron_Name(relu3_1_dw);
			Declear_Opration(baseconv, conv3_1_em);
			Neuron_Name(conv3_1_em);
			Declear_Opration(eltwise, res3_1);
			Neuron_Name(res3_1);
			Declear_Opration(baseconv, conv3_2_ex);
			Neuron_Name(conv3_2_ex);
			Declear_Opration(prelu, relu3_2_ex);
			Neuron_Name(relu3_2_ex);
			Declear_Opration(baseconv, conv3_2_dw);
			Neuron_Name(conv3_2_dw);
			Declear_Opration(prelu, relu3_2_dw);
			Neuron_Name(relu3_2_dw);
			Declear_Opration(baseconv, conv3_2_em);
			Neuron_Name(conv3_2_em);
			Declear_Opration(eltwise, res3_2);
			Neuron_Name(res3_2);
			Declear_Opration(baseconv, conv3_3_ex);
			Neuron_Name(conv3_3_ex);
			Declear_Opration(prelu, relu3_3_ex);
			Neuron_Name(relu3_3_ex);
			Declear_Opration(baseconv, conv3_3_dw);
			Neuron_Name(conv3_3_dw);
			Declear_Opration(prelu, relu3_3_dw);
			Neuron_Name(relu3_3_dw);
			Declear_Opration(baseconv, conv3_3_em);
			Neuron_Name(conv3_3_em);
			Declear_Opration(eltwise, res3_3);
			Neuron_Name(res3_3);
			Declear_Opration(baseconv, conv3_4_ex);
			Neuron_Name(conv3_4_ex);
			Declear_Opration(prelu, relu3_4_ex);
			Neuron_Name(relu3_4_ex);
			Declear_Opration(baseconv, conv3_4_dw);
			Neuron_Name(conv3_4_dw);
			Declear_Opration(prelu, relu3_4_dw);
			Neuron_Name(relu3_4_dw);
			Declear_Opration(baseconv, conv3_4_em);
			Neuron_Name(conv3_4_em);
			Declear_Opration(eltwise, res3_4);
			Neuron_Name(res3_4);
			Declear_Opration(baseconv, conv3_5_ex);
			Neuron_Name(conv3_5_ex);
			Declear_Opration(prelu, relu3_5_ex);
			Neuron_Name(relu3_5_ex);
			Declear_Opration(baseconv, conv3_5_dw);
			Neuron_Name(conv3_5_dw);
			Declear_Opration(prelu, relu3_5_dw);
			Neuron_Name(relu3_5_dw);
			Declear_Opration(baseconv, conv3_5_em);
			Neuron_Name(conv3_5_em);
			Declear_Opration(eltwise, res3_5);
			Neuron_Name(res3_5);
			Declear_Opration(baseconv, conv3_6_ex);
			Neuron_Name(conv3_6_ex);
			Declear_Opration(prelu, relu3_6_ex);
			Neuron_Name(relu3_6_ex);
			Declear_Opration(baseconv, conv3_6_dw);
			Neuron_Name(conv3_6_dw);
			Declear_Opration(prelu, relu3_6_dw);
			Neuron_Name(relu3_6_dw);
			Declear_Opration(baseconv, conv3_6_em);
			Neuron_Name(conv3_6_em);
			Declear_Opration(eltwise, res3_6);
			Neuron_Name(res3_6);
			Declear_Opration(baseconv, conv4_ex);
			Neuron_Name(conv4_ex);
			Declear_Opration(prelu, relu4_ex);
			Neuron_Name(relu4_ex);
			Declear_Opration(baseconv, conv4_dw);
			Neuron_Name(conv4_dw);
			Declear_Opration(prelu, relu4_dw);
			Neuron_Name(relu4_dw);
			Declear_Opration(baseconv, conv4_em);
			Neuron_Name(conv4_em);
			Declear_Opration(baseconv, conv4_1_ex);
			Neuron_Name(conv4_1_ex);
			Declear_Opration(prelu, relu4_1_ex);
			Neuron_Name(relu4_1_ex);
			Declear_Opration(baseconv, conv4_1_dw);
			Neuron_Name(conv4_1_dw);
			Declear_Opration(prelu, relu4_1_dw);
			Neuron_Name(relu4_1_dw);
			Declear_Opration(baseconv, conv4_1_em);
			Neuron_Name(conv4_1_em);
			Declear_Opration(eltwise, res4_1);
			Neuron_Name(res4_1);
			Declear_Opration(baseconv, conv4_2_ex);
			Neuron_Name(conv4_2_ex);
			Declear_Opration(prelu, relu4_2_ex);
			Neuron_Name(relu4_2_ex);
			Declear_Opration(baseconv, conv4_2_dw);
			Neuron_Name(conv4_2_dw);
			Declear_Opration(prelu, relu4_2_dw);
			Neuron_Name(relu4_2_dw);
			Declear_Opration(baseconv, conv4_2_em);
			Neuron_Name(conv4_2_em);
			Declear_Opration(eltwise, res4_2);
			Neuron_Name(res4_2);
			Declear_Opration(baseconv, conv5_ex);
			Neuron_Name(conv5_ex);
			Declear_Opration(prelu, relu5_ex);
			Neuron_Name(relu5_ex);
			Declear_Opration(baseconv, conv5_dw);
			Neuron_Name(conv5_dw);
			Declear_Opration(inner_product, fc5);
			Neuron_Name(fc5);

#ifdef USE_CUDA
			cublasHandle_t cublas_handle_ = nullptr;
			void Forward_gpu_native(const std::shared_ptr<memory::tensor<float>> input_data);
#ifdef USE_CUDNN
			cudnnHandle_t cudnn_handle_ = nullptr;
			void Forward_gpu_cudnn(const std::shared_ptr<memory::tensor<float>> input_data);
#endif 
#endif
			void Forward_cpu(const std::shared_ptr<memory::tensor<float>> input_data);

		public:
			Unicorn_mobile(int device);
			virtual ~Unicorn_mobile();

			std::vector<std::vector<float> > Forward(const float* input_data, unsigned num, int order = 0);

			std::vector<std::vector<float> > Forward(const unsigned char* input_data, unsigned num, int order = 0);
		};
	}
}

#endif //!_UNICORN_MOBILE_HPP_
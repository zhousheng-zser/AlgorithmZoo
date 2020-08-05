#include "unicorn_mobile_mask.hpp"
#include "Primitives/memory.hpp"
#include "Primitives/simd_types.hpp"
#include "unicorn_mobile_mask_data.hpp"

#ifdef __ARM_NEON
#include "Excalibur/tensor_operation_cpu.hpp"
#endif

#include <iostream>
#include <vector>

using glasssix::memory::aligned_heap_free;

namespace glasssix
{
	namespace gaius
	{
		Unicorn_mobile_mask::Unicorn_mobile_mask(int device)
		{
			device_ = device;
			if (device_ >= 0)
			{
				int8_quantization_ = false;//do not use int8 in GPU mode
			}

#ifdef __ARM_NEON
			int8_quantization_ = false;//do not use int8 in ARM mode
#endif

#if SIMD_TYPE >= SIMDTYPE_SSE
			//use for Copy_Int8_Params
			std::shared_ptr<memory::tensor<float>> bottom_round_ = std::make_shared<memory::tensor<float>>(std::vector<int>{mm_align_size});
			float* bottom_round_data_ = bottom_round_->mutable_cpu_data();
#endif // SIMD_TYPE >= SIMDTYPE_SSE

			float quantize_level = INT_MAX;
			//copy float32_data directly
			Copy_Params(conv1_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv1_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu1_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv1_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_1_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_2_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_3_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu2_4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv2_4_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_1_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_2_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_3_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_3_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_3_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_4_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_5_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_5_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_5_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_6_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu3_6_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv3_6_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_1_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_1_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_1_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_2_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu4_2_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_em_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv4_2_em_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv5_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv5_ex_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(relu5_ex_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv5_dw_weights, Unicorn_mobile_mask, quantize_level);
			Copy_Params(conv5_dw_bias, Unicorn_mobile_mask, quantize_level);
			Copy_Params(fc5_weights, Unicorn_mobile_mask, quantize_level);

#ifdef USE_CUDA
			CUBLAS_CHECK(cublasCreate(&cublas_handle_));
#ifdef USE_CUDNN
			CUDNN_CHECK(cudnnCreate(&cudnn_handle_));
			cudnn_ready_ = true;
#endif
#endif

#ifdef __ARM_NEON
			Init_Conv_arm_Params(conv1, 3, 64, 1, 3, 2, 1, true);//nchw:1*3*128*128->1*64*64*64
			Init_PReLU_arm_Params(relu1, 64, false, false);//nchw:1*64*64*64->1*64*64*64
			Init_Conv_arm_Params(conv1_dw, 64, 64, 64, 3, 1, 1, true);//nchw:1*64*64*64->1*64*64*64
			Init_PReLU_arm_Params(relu1_dw, 64, false, false);//nchw:1*64*64*64->1*64*64*64
			Init_Conv_arm_Params(conv2_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*64*64->1*128*64*64
			Init_PReLU_arm_Params(relu2_ex, 128, false, false);//nchw:1*128*64*64->1*128*64*64
			Init_Conv_arm_Params(conv2_dw, 128, 128, 128, 3, 2, 1, true);//nchw:1*128*64*64->1*128*64*64
			Init_PReLU_arm_Params(relu2_dw, 128, false, false);//nchw:1*128*64*64->1*128*32*32
			Init_Conv_arm_Params(conv2_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Conv_arm_Params(conv2_1_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_1_ex, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_1_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_1_dw, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_1_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_arm_Params(res2_1, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_arm_Params(conv2_2_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_2_ex, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_2_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_2_dw, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_2_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_arm_Params(res2_2, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_arm_Params(conv2_3_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_3_ex, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_3_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_3_dw, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_3_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_arm_Params(res2_3, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_arm_Params(conv2_4_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_4_ex, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_4_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_arm_Params(relu2_4_dw, 128, false, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_arm_Params(conv2_4_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_arm_Params(res2_4, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_arm_Params(conv3_ex, 64, 256, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*256*32*32
			Init_PReLU_arm_Params(relu3_ex, 256, false, false);//nchw:1*256*32*32->1*256*32*32
			Init_Conv_arm_Params(conv3_dw, 256, 256, 256, 3, 2, 1, true);//nchw:1*256*32*32->1*256*16*16
			Init_PReLU_arm_Params(relu3_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_1_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_1_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_1_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_1_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_1_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_1, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_2_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_2_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_2_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_2_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_2_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_2, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_3_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_3_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_3_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_3_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_3_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_3, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_4_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_4_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_4_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_4_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_4_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_4, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_5_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_5_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_5_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_5_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_5_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_5, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv3_6_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_6_ex, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_6_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_arm_Params(relu3_6_dw, 256, false, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_arm_Params(conv3_6_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_arm_Params(res3_6, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_arm_Params(conv4_ex, 128, 512, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*512*16*16
			Init_PReLU_arm_Params(relu4_ex, 512, false, false);//nchw:1*512*16*16->1*512*16*16
			Init_Conv_arm_Params(conv4_dw, 512, 512, 512, 3, 2, 1, true);//nchw:1*512*16*16->1*512*8*8
			Init_PReLU_arm_Params(relu4_dw, 512, false, false);//nchw:1*512*8*8->1*512*8*8
			Init_Conv_arm_Params(conv4_em, 512, 128, 1, 1, 1, 0, true);//nchw:1*512*8*8->1*128*8*8
			Init_Conv_arm_Params(conv4_1_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*256*8*8
			Init_PReLU_arm_Params(relu4_1_ex, 256, false, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_arm_Params(conv4_1_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*8*8->1*256*8*8
			Init_PReLU_arm_Params(relu4_1_dw, 256, false, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_arm_Params(conv4_1_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*8*8->1*128*8*8
			Init_Eltwise_arm_Params(res4_1, 0);//nchw:1*128*8*8->1*128*8*8
			Init_Conv_arm_Params(conv4_2_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*256*8*8
			Init_PReLU_arm_Params(relu4_2_ex, 256, false, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_arm_Params(conv4_2_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*8*8->1*256*8*8
			Init_PReLU_arm_Params(relu4_2_dw, 256, false, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_arm_Params(conv4_2_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*8*8->1*128*8*8
			Init_Eltwise_arm_Params(res4_2, 0);//nchw:1*128*8*8->1*128*8*8
			Init_Conv_arm_Params(conv5_ex, 128, 512, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*512*8*8
			Init_PReLU_arm_Params(relu5_ex, 512, false, false);//nchw:1*512*8*8->1*512*8*8
			Init_Conv_arm_Params(conv5_dw, 512, 512, 512, 8, 1, 0, true);//nchw:1*512*8*8->1*512*1*1
			Init_InnerProduct_arm_Params(fc5, 512, 1, 1, 128, false);//nchw:1*512*1*1->1*128*1*1
#else
			Init_Conv_Params(conv1, 3, 64, 1, 3, 2, 1, true);//nchw:1*3*128*128->1*64*64*64
			Init_PReLU_Params(relu1, 64, false);//nchw:1*64*64*64->1*64*64*64
			Init_Conv_Params(conv1_dw, 64, 64, 64, 3, 1, 1, true);//nchw:1*64*64*64->1*64*64*64
			Init_PReLU_Params(relu1_dw, 64, false);//nchw:1*64*64*64->1*64*64*64
			Init_Conv_Params(conv2_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*64*64->1*128*64*64
			Init_PReLU_Params(relu2_ex, 128, false);//nchw:1*128*64*64->1*128*64*64
			Init_Conv_Params(conv2_dw, 128, 128, 128, 3, 2, 1, true);//nchw:1*128*64*64->1*128*32*32
			Init_PReLU_Params(relu2_dw, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Conv_Params(conv2_1_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_Params(relu2_1_ex, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_1_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_Params(relu2_1_dw, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_1_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_Params(res2_1, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_Params(conv2_2_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_Params(relu2_2_ex, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_2_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_Params(relu2_2_dw, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_2_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_Params(res2_2, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_Params(conv2_3_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_Params(relu2_3_ex, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_3_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_Params(relu2_3_dw, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_3_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_Params(res2_3, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_Params(conv2_4_ex, 64, 128, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*128*32*32
			Init_PReLU_Params(relu2_4_ex, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_4_dw, 128, 128, 128, 3, 1, 1, true);//nchw:1*128*32*32->1*128*32*32
			Init_PReLU_Params(relu2_4_dw, 128, false);//nchw:1*128*32*32->1*128*32*32
			Init_Conv_Params(conv2_4_em, 128, 64, 1, 1, 1, 0, true);//nchw:1*128*32*32->1*64*32*32
			Init_Eltwise_Params(res2_4, 0);//nchw:1*64*32*32->1*64*32*32
			Init_Conv_Params(conv3_ex, 64, 256, 1, 1, 1, 0, true);//nchw:1*64*32*32->1*256*32*32
			Init_PReLU_Params(relu3_ex, 256, false);//nchw:1*256*32*32->1*256*32*32
			Init_Conv_Params(conv3_dw, 256, 256, 256, 3, 2, 1, true);//nchw:1*256*32*32->1*256*16*16
			Init_PReLU_Params(relu3_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Conv_Params(conv3_1_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_1_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_1_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_1_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_1_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_1, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv3_2_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_2_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_2_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_2_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_2_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_2, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv3_3_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_3_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_3_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_3_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_3_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_3, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv3_4_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_4_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_4_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_4_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_4_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_4, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv3_5_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_5_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_5_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_5_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_5_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_5, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv3_6_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*256*16*16
			Init_PReLU_Params(relu3_6_ex, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_6_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*16*16->1*256*16*16
			Init_PReLU_Params(relu3_6_dw, 256, false);//nchw:1*256*16*16->1*256*16*16
			Init_Conv_Params(conv3_6_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*16*16->1*128*16*16
			Init_Eltwise_Params(res3_6, 0);//nchw:1*128*16*16->1*128*16*16
			Init_Conv_Params(conv4_ex, 128, 512, 1, 1, 1, 0, true);//nchw:1*128*16*16->1*512*16*16
			Init_PReLU_Params(relu4_ex, 512, false);//nchw:1*512*16*16->1*512*16*16
			Init_Conv_Params(conv4_dw, 512, 512, 512, 3, 2, 1, true);//nchw:1*512*16*16->1*512*8*8
			Init_PReLU_Params(relu4_dw, 512, false);//nchw:1*512*8*8->1*512*8*8
			Init_Conv_Params(conv4_em, 512, 128, 1, 1, 1, 0, true);//nchw:1*512*8*8->1*128*8*8
			Init_Conv_Params(conv4_1_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*256*8*8
			Init_PReLU_Params(relu4_1_ex, 256, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_Params(conv4_1_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*8*8->1*256*8*8
			Init_PReLU_Params(relu4_1_dw, 256, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_Params(conv4_1_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*8*8->1*128*8*8
			Init_Eltwise_Params(res4_1, 0);//nchw:1*128*8*8->1*128*8*8
			Init_Conv_Params(conv4_2_ex, 128, 256, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*256*8*8
			Init_PReLU_Params(relu4_2_ex, 256, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_Params(conv4_2_dw, 256, 256, 256, 3, 1, 1, true);//nchw:1*256*8*8->1*256*8*8
			Init_PReLU_Params(relu4_2_dw, 256, false);//nchw:1*256*8*8->1*256*8*8
			Init_Conv_Params(conv4_2_em, 256, 128, 1, 1, 1, 0, true);//nchw:1*256*8*8->1*128*8*8
			Init_Eltwise_Params(res4_2, 0);//nchw:1*128*8*8->1*128*8*8
			Init_Conv_Params(conv5_ex, 128, 512, 1, 1, 1, 0, true);//nchw:1*128*8*8->1*512*8*8
			Init_PReLU_Params(relu5_ex, 512, false);//nchw:1*512*8*8->1*512*8*8
			Init_Conv_Params(conv5_dw, 512, 512, 512, 8, 1, 0, true);//nchw:1*512*8*8->1*512*1*1
			Init_InnerProduct_Params(fc5, 512, 1, 1, 128, false);//nchw:1*512*1*1->1*128*1*1
#endif
		}

		Unicorn_mobile_mask::~Unicorn_mobile_mask()
		{
			delete conv1;
			delete relu1;
			delete conv1_dw;
			delete relu1_dw;
			delete conv2_ex;
			delete relu2_ex;
			delete conv2_dw;
			delete relu2_dw;
			delete conv2_em;
			delete conv2_1_ex;
			delete relu2_1_ex;
			delete conv2_1_dw;
			delete relu2_1_dw;
			delete conv2_1_em;
			delete res2_1;
			delete conv2_2_ex;
			delete relu2_2_ex;
			delete conv2_2_dw;
			delete relu2_2_dw;
			delete conv2_2_em;
			delete res2_2;
			delete conv2_3_ex;
			delete relu2_3_ex;
			delete conv2_3_dw;
			delete relu2_3_dw;
			delete conv2_3_em;
			delete res2_3;
			delete conv2_4_ex;
			delete relu2_4_ex;
			delete conv2_4_dw;
			delete relu2_4_dw;
			delete conv2_4_em;
			delete res2_4;
			delete conv3_ex;
			delete relu3_ex;
			delete conv3_dw;
			delete relu3_dw;
			delete conv3_em;
			delete conv3_1_ex;
			delete relu3_1_ex;
			delete conv3_1_dw;
			delete relu3_1_dw;
			delete conv3_1_em;
			delete res3_1;
			delete conv3_2_ex;
			delete relu3_2_ex;
			delete conv3_2_dw;
			delete relu3_2_dw;
			delete conv3_2_em;
			delete res3_2;
			delete conv3_3_ex;
			delete relu3_3_ex;
			delete conv3_3_dw;
			delete relu3_3_dw;
			delete conv3_3_em;
			delete res3_3;
			delete conv3_4_ex;
			delete relu3_4_ex;
			delete conv3_4_dw;
			delete relu3_4_dw;
			delete conv3_4_em;
			delete res3_4;
			delete conv3_5_ex;
			delete relu3_5_ex;
			delete conv3_5_dw;
			delete relu3_5_dw;
			delete conv3_5_em;
			delete res3_5;
			delete conv3_6_ex;
			delete relu3_6_ex;
			delete conv3_6_dw;
			delete relu3_6_dw;
			delete conv3_6_em;
			delete res3_6;
			delete conv4_ex;
			delete relu4_ex;
			delete conv4_dw;
			delete relu4_dw;
			delete conv4_em;
			delete conv4_1_ex;
			delete relu4_1_ex;
			delete conv4_1_dw;
			delete relu4_1_dw;
			delete conv4_1_em;
			delete res4_1;
			delete conv4_2_ex;
			delete relu4_2_ex;
			delete conv4_2_dw;
			delete relu4_2_dw;
			delete conv4_2_em;
			delete res4_2;
			delete conv5_ex;
			delete relu5_ex;
			delete conv5_dw;
			delete fc5;

			//conv_weights and bias free automatically, prelu_weights need to free explicitly
			aligned_heap_free(relu1_weights);
			aligned_heap_free(relu1_dw_weights);
			aligned_heap_free(relu2_ex_weights);
			aligned_heap_free(relu2_dw_weights);
			aligned_heap_free(relu2_1_ex_weights);
			aligned_heap_free(relu2_1_dw_weights);
			aligned_heap_free(relu2_2_ex_weights);
			aligned_heap_free(relu2_2_dw_weights);
			aligned_heap_free(relu2_3_ex_weights);
			aligned_heap_free(relu2_3_dw_weights);
			aligned_heap_free(relu2_4_ex_weights);
			aligned_heap_free(relu2_4_dw_weights);
			aligned_heap_free(relu3_ex_weights);
			aligned_heap_free(relu3_dw_weights);
			aligned_heap_free(relu3_1_ex_weights);
			aligned_heap_free(relu3_1_dw_weights);
			aligned_heap_free(relu3_2_ex_weights);
			aligned_heap_free(relu3_2_dw_weights);
			aligned_heap_free(relu3_3_ex_weights);
			aligned_heap_free(relu3_3_dw_weights);
			aligned_heap_free(relu3_4_ex_weights);
			aligned_heap_free(relu3_4_dw_weights);
			aligned_heap_free(relu3_5_ex_weights);
			aligned_heap_free(relu3_5_dw_weights);
			aligned_heap_free(relu3_6_ex_weights);
			aligned_heap_free(relu3_6_dw_weights);
			aligned_heap_free(relu4_ex_weights);
			aligned_heap_free(relu4_dw_weights);
			aligned_heap_free(relu4_1_ex_weights);
			aligned_heap_free(relu4_1_dw_weights);
			aligned_heap_free(relu4_2_ex_weights);
			aligned_heap_free(relu4_2_dw_weights);
			aligned_heap_free(relu5_ex_weights);

#ifdef USE_CUDA
			if (cublas_handle_)
			{
				CUBLAS_CHECK(cublasDestroy(cublas_handle_));
			}
#ifdef USE_CUDNN
			if (cudnn_handle_)
			{
				CUDNN_CHECK(cudnnDestroy(cudnn_handle_));
			}
#endif
#endif
		}

		void Unicorn_mobile_mask::Forward_cpu(const std::shared_ptr<memory::tensor<float>> input_data)
		{
			conv1->Forward(input_data, conv1_top_data);
			relu1->Forward_cpu(conv1_top_data);
			conv1_dw->Forward(conv1_top_data, conv1_dw_top_data);
			relu1_dw->Forward_cpu(conv1_dw_top_data);
			conv2_ex->Forward(conv1_dw_top_data, conv2_ex_top_data);
			relu2_ex->Forward_cpu(conv2_ex_top_data);
			conv2_dw->Forward(conv2_ex_top_data, conv2_dw_top_data);
			relu2_dw->Forward_cpu(conv2_dw_top_data);
			conv2_em->Forward(conv2_dw_top_data, conv2_em_top_data);
			conv2_1_ex->Forward(conv2_em_top_data, conv2_1_ex_top_data);
			relu2_1_ex->Forward_cpu(conv2_1_ex_top_data);
			conv2_1_dw->Forward(conv2_1_ex_top_data, conv2_1_dw_top_data);
			relu2_1_dw->Forward_cpu(conv2_1_dw_top_data);
			conv2_1_em->Forward(conv2_1_dw_top_data, conv2_1_em_top_data);
			res2_1->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{conv2_em_top_data, conv2_1_em_top_data}, res2_1_top_data);
			conv2_2_ex->Forward(res2_1_top_data, conv2_2_ex_top_data);
			relu2_2_ex->Forward_cpu(conv2_2_ex_top_data);
			conv2_2_dw->Forward(conv2_2_ex_top_data, conv2_2_dw_top_data);
			relu2_2_dw->Forward_cpu(conv2_2_dw_top_data);
			conv2_2_em->Forward(conv2_2_dw_top_data, conv2_2_em_top_data);
			res2_2->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res2_1_top_data, conv2_2_em_top_data}, res2_2_top_data);
			conv2_3_ex->Forward(res2_2_top_data, conv2_3_ex_top_data);
			relu2_3_ex->Forward_cpu(conv2_3_ex_top_data);
			conv2_3_dw->Forward(conv2_3_ex_top_data, conv2_3_dw_top_data);
			relu2_3_dw->Forward_cpu(conv2_3_dw_top_data);
			conv2_3_em->Forward(conv2_3_dw_top_data, conv2_3_em_top_data);
			res2_3->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res2_2_top_data, conv2_3_em_top_data}, res2_3_top_data);
			conv2_4_ex->Forward(res2_3_top_data, conv2_4_ex_top_data);
			relu2_4_ex->Forward_cpu(conv2_4_ex_top_data);
			conv2_4_dw->Forward(conv2_4_ex_top_data, conv2_4_dw_top_data);
			relu2_4_dw->Forward_cpu(conv2_4_dw_top_data);
			conv2_4_em->Forward(conv2_4_dw_top_data, conv2_4_em_top_data);
			res2_4->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res2_3_top_data, conv2_4_em_top_data}, res2_4_top_data);
			conv3_ex->Forward(res2_4_top_data, conv3_ex_top_data);
			relu3_ex->Forward_cpu(conv3_ex_top_data);
			conv3_dw->Forward(conv3_ex_top_data, conv3_dw_top_data);
			relu3_dw->Forward_cpu(conv3_dw_top_data);
			conv3_em->Forward(conv3_dw_top_data, conv3_em_top_data);
			conv3_1_ex->Forward(conv3_em_top_data, conv3_1_ex_top_data);
			relu3_1_ex->Forward_cpu(conv3_1_ex_top_data);
			conv3_1_dw->Forward(conv3_1_ex_top_data, conv3_1_dw_top_data);
			relu3_1_dw->Forward_cpu(conv3_1_dw_top_data);
			conv3_1_em->Forward(conv3_1_dw_top_data, conv3_1_em_top_data);
			res3_1->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{conv3_em_top_data, conv3_1_em_top_data}, res3_1_top_data);
			conv3_2_ex->Forward(res3_1_top_data, conv3_2_ex_top_data);
			relu3_2_ex->Forward_cpu(conv3_2_ex_top_data);
			conv3_2_dw->Forward(conv3_2_ex_top_data, conv3_2_dw_top_data);
			relu3_2_dw->Forward_cpu(conv3_2_dw_top_data);
			conv3_2_em->Forward(conv3_2_dw_top_data, conv3_2_em_top_data);
			res3_2->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res3_1_top_data, conv3_2_em_top_data}, res3_2_top_data);
			conv3_3_ex->Forward(res3_2_top_data, conv3_3_ex_top_data);
			relu3_3_ex->Forward_cpu(conv3_3_ex_top_data);
			conv3_3_dw->Forward(conv3_3_ex_top_data, conv3_3_dw_top_data);
			relu3_3_dw->Forward_cpu(conv3_3_dw_top_data);
			conv3_3_em->Forward(conv3_3_dw_top_data, conv3_3_em_top_data);
			res3_3->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res3_2_top_data, conv3_3_em_top_data}, res3_3_top_data);
			conv3_4_ex->Forward(res3_3_top_data, conv3_4_ex_top_data);
			relu3_4_ex->Forward_cpu(conv3_4_ex_top_data);
			conv3_4_dw->Forward(conv3_4_ex_top_data, conv3_4_dw_top_data);
			relu3_4_dw->Forward_cpu(conv3_4_dw_top_data);
			conv3_4_em->Forward(conv3_4_dw_top_data, conv3_4_em_top_data);
			res3_4->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res3_3_top_data, conv3_4_em_top_data}, res3_4_top_data);
			conv3_5_ex->Forward(res3_4_top_data, conv3_5_ex_top_data);
			relu3_5_ex->Forward_cpu(conv3_5_ex_top_data);
			conv3_5_dw->Forward(conv3_5_ex_top_data, conv3_5_dw_top_data);
			relu3_5_dw->Forward_cpu(conv3_5_dw_top_data);
			conv3_5_em->Forward(conv3_5_dw_top_data, conv3_5_em_top_data);
			res3_5->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res3_4_top_data, conv3_5_em_top_data}, res3_5_top_data);
			conv3_6_ex->Forward(res3_5_top_data, conv3_6_ex_top_data);
			relu3_6_ex->Forward_cpu(conv3_6_ex_top_data);
			conv3_6_dw->Forward(conv3_6_ex_top_data, conv3_6_dw_top_data);
			relu3_6_dw->Forward_cpu(conv3_6_dw_top_data);
			conv3_6_em->Forward(conv3_6_dw_top_data, conv3_6_em_top_data);
			res3_6->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res3_5_top_data, conv3_6_em_top_data}, res3_6_top_data);
			conv4_ex->Forward(res3_6_top_data, conv4_ex_top_data);
			relu4_ex->Forward_cpu(conv4_ex_top_data);
			conv4_dw->Forward(conv4_ex_top_data, conv4_dw_top_data);
			relu4_dw->Forward_cpu(conv4_dw_top_data);
			conv4_em->Forward(conv4_dw_top_data, conv4_em_top_data);
			conv4_1_ex->Forward(conv4_em_top_data, conv4_1_ex_top_data);
			relu4_1_ex->Forward_cpu(conv4_1_ex_top_data);
			conv4_1_dw->Forward(conv4_1_ex_top_data, conv4_1_dw_top_data);
			relu4_1_dw->Forward_cpu(conv4_1_dw_top_data);
			conv4_1_em->Forward(conv4_1_dw_top_data, conv4_1_em_top_data);
			res4_1->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{conv4_em_top_data, conv4_1_em_top_data}, res4_1_top_data);
			conv4_2_ex->Forward(res4_1_top_data, conv4_2_ex_top_data);
			relu4_2_ex->Forward_cpu(conv4_2_ex_top_data);
			conv4_2_dw->Forward(conv4_2_ex_top_data, conv4_2_dw_top_data);
			relu4_2_dw->Forward_cpu(conv4_2_dw_top_data);
			conv4_2_em->Forward(conv4_2_dw_top_data, conv4_2_em_top_data);
			res4_2->Forward_cpu(std::vector<std::shared_ptr<memory::tensor<float>>>{res4_1_top_data, conv4_2_em_top_data}, res4_2_top_data);
			conv5_ex->Forward(res4_2_top_data, conv5_ex_top_data);
			relu5_ex->Forward_cpu(conv5_ex_top_data);
			conv5_dw->Forward(conv5_ex_top_data, conv5_dw_top_data);
			fc5->Forward_cpu(conv5_dw_top_data, fc5_top_data);
		}

#ifdef USE_CUDA
		void Unicorn_mobile_mask::Forward_gpu_native(const std::shared_ptr<memory::tensor<float>> input_data)
		{
			conv1->Forward(cublas_handle_, input_data, conv1_top_data);
			relu1->Forward_gpu_native(conv1_top_data);
			conv1_dw->Forward(cublas_handle_, conv1_top_data, conv1_dw_top_data);
			relu1_dw->Forward_gpu_native(conv1_dw_top_data);
			conv2_ex->Forward(cublas_handle_, conv1_dw_top_data, conv2_ex_top_data);
			relu2_ex->Forward_gpu_native(conv2_ex_top_data);
			conv2_dw->Forward(cublas_handle_, conv2_ex_top_data, conv2_dw_top_data);
			relu2_dw->Forward_gpu_native(conv2_dw_top_data);
			conv2_em->Forward(cublas_handle_, conv2_dw_top_data, conv2_em_top_data);
			conv2_1_ex->Forward(cublas_handle_, conv2_em_top_data, conv2_1_ex_top_data);
			relu2_1_ex->Forward_gpu_native(conv2_1_ex_top_data);
			conv2_1_dw->Forward(cublas_handle_, conv2_1_ex_top_data, conv2_1_dw_top_data);
			relu2_1_dw->Forward_gpu_native(conv2_1_dw_top_data);
			conv2_1_em->Forward(cublas_handle_, conv2_1_dw_top_data, conv2_1_em_top_data);
			res2_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv2_em_top_data, conv2_1_em_top_data}, res2_1_top_data);
			conv2_2_ex->Forward(cublas_handle_, res2_1_top_data, conv2_2_ex_top_data);
			relu2_2_ex->Forward_gpu_native(conv2_2_ex_top_data);
			conv2_2_dw->Forward(cublas_handle_, conv2_2_ex_top_data, conv2_2_dw_top_data);
			relu2_2_dw->Forward_gpu_native(conv2_2_dw_top_data);
			conv2_2_em->Forward(cublas_handle_, conv2_2_dw_top_data, conv2_2_em_top_data);
			res2_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_1_top_data, conv2_2_em_top_data}, res2_2_top_data);
			conv2_3_ex->Forward(cublas_handle_, res2_2_top_data, conv2_3_ex_top_data);
			relu2_3_ex->Forward_gpu_native(conv2_3_ex_top_data);
			conv2_3_dw->Forward(cublas_handle_, conv2_3_ex_top_data, conv2_3_dw_top_data);
			relu2_3_dw->Forward_gpu_native(conv2_3_dw_top_data);
			conv2_3_em->Forward(cublas_handle_, conv2_3_dw_top_data, conv2_3_em_top_data);
			res2_3->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_2_top_data, conv2_3_em_top_data}, res2_3_top_data);
			conv2_4_ex->Forward(cublas_handle_, res2_3_top_data, conv2_4_ex_top_data);
			relu2_4_ex->Forward_gpu_native(conv2_4_ex_top_data);
			conv2_4_dw->Forward(cublas_handle_, conv2_4_ex_top_data, conv2_4_dw_top_data);
			relu2_4_dw->Forward_gpu_native(conv2_4_dw_top_data);
			conv2_4_em->Forward(cublas_handle_, conv2_4_dw_top_data, conv2_4_em_top_data);
			res2_4->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_3_top_data, conv2_4_em_top_data}, res2_4_top_data);
			conv3_ex->Forward(cublas_handle_, res2_4_top_data, conv3_ex_top_data);
			relu3_ex->Forward_gpu_native(conv3_ex_top_data);
			conv3_dw->Forward(cublas_handle_, conv3_ex_top_data, conv3_dw_top_data);
			relu3_dw->Forward_gpu_native(conv3_dw_top_data);
			conv3_em->Forward(cublas_handle_, conv3_dw_top_data, conv3_em_top_data);
			conv3_1_ex->Forward(cublas_handle_, conv3_em_top_data, conv3_1_ex_top_data);
			relu3_1_ex->Forward_gpu_native(conv3_1_ex_top_data);
			conv3_1_dw->Forward(cublas_handle_, conv3_1_ex_top_data, conv3_1_dw_top_data);
			relu3_1_dw->Forward_gpu_native(conv3_1_dw_top_data);
			conv3_1_em->Forward(cublas_handle_, conv3_1_dw_top_data, conv3_1_em_top_data);
			res3_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv3_em_top_data, conv3_1_em_top_data}, res3_1_top_data);
			conv3_2_ex->Forward(cublas_handle_, res3_1_top_data, conv3_2_ex_top_data);
			relu3_2_ex->Forward_gpu_native(conv3_2_ex_top_data);
			conv3_2_dw->Forward(cublas_handle_, conv3_2_ex_top_data, conv3_2_dw_top_data);
			relu3_2_dw->Forward_gpu_native(conv3_2_dw_top_data);
			conv3_2_em->Forward(cublas_handle_, conv3_2_dw_top_data, conv3_2_em_top_data);
			res3_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_1_top_data, conv3_2_em_top_data}, res3_2_top_data);
			conv3_3_ex->Forward(cublas_handle_, res3_2_top_data, conv3_3_ex_top_data);
			relu3_3_ex->Forward_gpu_native(conv3_3_ex_top_data);
			conv3_3_dw->Forward(cublas_handle_, conv3_3_ex_top_data, conv3_3_dw_top_data);
			relu3_3_dw->Forward_gpu_native(conv3_3_dw_top_data);
			conv3_3_em->Forward(cublas_handle_, conv3_3_dw_top_data, conv3_3_em_top_data);
			res3_3->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_2_top_data, conv3_3_em_top_data}, res3_3_top_data);
			conv3_4_ex->Forward(cublas_handle_, res3_3_top_data, conv3_4_ex_top_data);
			relu3_4_ex->Forward_gpu_native(conv3_4_ex_top_data);
			conv3_4_dw->Forward(cublas_handle_, conv3_4_ex_top_data, conv3_4_dw_top_data);
			relu3_4_dw->Forward_gpu_native(conv3_4_dw_top_data);
			conv3_4_em->Forward(cublas_handle_, conv3_4_dw_top_data, conv3_4_em_top_data);
			res3_4->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_3_top_data, conv3_4_em_top_data}, res3_4_top_data);
			conv3_5_ex->Forward(cublas_handle_, res3_4_top_data, conv3_5_ex_top_data);
			relu3_5_ex->Forward_gpu_native(conv3_5_ex_top_data);
			conv3_5_dw->Forward(cublas_handle_, conv3_5_ex_top_data, conv3_5_dw_top_data);
			relu3_5_dw->Forward_gpu_native(conv3_5_dw_top_data);
			conv3_5_em->Forward(cublas_handle_, conv3_5_dw_top_data, conv3_5_em_top_data);
			res3_5->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_4_top_data, conv3_5_em_top_data}, res3_5_top_data);
			conv3_6_ex->Forward(cublas_handle_, res3_5_top_data, conv3_6_ex_top_data);
			relu3_6_ex->Forward_gpu_native(conv3_6_ex_top_data);
			conv3_6_dw->Forward(cublas_handle_, conv3_6_ex_top_data, conv3_6_dw_top_data);
			relu3_6_dw->Forward_gpu_native(conv3_6_dw_top_data);
			conv3_6_em->Forward(cublas_handle_, conv3_6_dw_top_data, conv3_6_em_top_data);
			res3_6->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_5_top_data, conv3_6_em_top_data}, res3_6_top_data);
			conv4_ex->Forward(cublas_handle_, res3_6_top_data, conv4_ex_top_data);
			relu4_ex->Forward_gpu_native(conv4_ex_top_data);
			conv4_dw->Forward(cublas_handle_, conv4_ex_top_data, conv4_dw_top_data);
			relu4_dw->Forward_gpu_native(conv4_dw_top_data);
			conv4_em->Forward(cublas_handle_, conv4_dw_top_data, conv4_em_top_data);
			conv4_1_ex->Forward(cublas_handle_, conv4_em_top_data, conv4_1_ex_top_data);
			relu4_1_ex->Forward_gpu_native(conv4_1_ex_top_data);
			conv4_1_dw->Forward(cublas_handle_, conv4_1_ex_top_data, conv4_1_dw_top_data);
			relu4_1_dw->Forward_gpu_native(conv4_1_dw_top_data);
			conv4_1_em->Forward(cublas_handle_, conv4_1_dw_top_data, conv4_1_em_top_data);
			res4_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv4_em_top_data, conv4_1_em_top_data}, res4_1_top_data);
			conv4_2_ex->Forward(cublas_handle_, res4_1_top_data, conv4_2_ex_top_data);
			relu4_2_ex->Forward_gpu_native(conv4_2_ex_top_data);
			conv4_2_dw->Forward(cublas_handle_, conv4_2_ex_top_data, conv4_2_dw_top_data);
			relu4_2_dw->Forward_gpu_native(conv4_2_dw_top_data);
			conv4_2_em->Forward(cublas_handle_, conv4_2_dw_top_data, conv4_2_em_top_data);
			res4_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res4_1_top_data, conv4_2_em_top_data}, res4_2_top_data);
			conv5_ex->Forward(cublas_handle_, res4_2_top_data, conv5_ex_top_data);
			relu5_ex->Forward_gpu_native(conv5_ex_top_data);
			conv5_dw->Forward(cublas_handle_, conv5_ex_top_data, conv5_dw_top_data);
			fc5->Forward_gpu_native(cublas_handle_, conv5_dw_top_data, fc5_top_data);
		}
#ifdef USE_CUDNN
		void Unicorn_mobile_mask::Forward_gpu_cudnn(const std::shared_ptr<memory::tensor<float>> input_data)
		{
			conv1->Forward(cudnn_handle_, input_data, conv1_top_data);
			relu1->Forward_gpu_native(conv1_top_data);
			conv1_dw->Forward(cudnn_handle_, conv1_top_data, conv1_dw_top_data);
			relu1_dw->Forward_gpu_native(conv1_dw_top_data);
			conv2_ex->Forward(cudnn_handle_, conv1_dw_top_data, conv2_ex_top_data);
			relu2_ex->Forward_gpu_native(conv2_ex_top_data);
			conv2_dw->Forward(cudnn_handle_, conv2_ex_top_data, conv2_dw_top_data);
			relu2_dw->Forward_gpu_native(conv2_dw_top_data);
			conv2_em->Forward(cudnn_handle_, conv2_dw_top_data, conv2_em_top_data);
			conv2_1_ex->Forward(cudnn_handle_, conv2_em_top_data, conv2_1_ex_top_data);
			relu2_1_ex->Forward_gpu_native(conv2_1_ex_top_data);
			conv2_1_dw->Forward(cudnn_handle_, conv2_1_ex_top_data, conv2_1_dw_top_data);
			relu2_1_dw->Forward_gpu_native(conv2_1_dw_top_data);
			conv2_1_em->Forward(cudnn_handle_, conv2_1_dw_top_data, conv2_1_em_top_data);
			res2_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv2_em_top_data, conv2_1_em_top_data}, res2_1_top_data);
			conv2_2_ex->Forward(cudnn_handle_, res2_1_top_data, conv2_2_ex_top_data);
			relu2_2_ex->Forward_gpu_native(conv2_2_ex_top_data);
			conv2_2_dw->Forward(cudnn_handle_, conv2_2_ex_top_data, conv2_2_dw_top_data);
			relu2_2_dw->Forward_gpu_native(conv2_2_dw_top_data);
			conv2_2_em->Forward(cudnn_handle_, conv2_2_dw_top_data, conv2_2_em_top_data);
			res2_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_1_top_data, conv2_2_em_top_data}, res2_2_top_data);
			conv2_3_ex->Forward(cudnn_handle_, res2_2_top_data, conv2_3_ex_top_data);
			relu2_3_ex->Forward_gpu_native(conv2_3_ex_top_data);
			conv2_3_dw->Forward(cudnn_handle_, conv2_3_ex_top_data, conv2_3_dw_top_data);
			relu2_3_dw->Forward_gpu_native(conv2_3_dw_top_data);
			conv2_3_em->Forward(cudnn_handle_, conv2_3_dw_top_data, conv2_3_em_top_data);
			res2_3->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_2_top_data, conv2_3_em_top_data}, res2_3_top_data);
			conv2_4_ex->Forward(cudnn_handle_, res2_3_top_data, conv2_4_ex_top_data);
			relu2_4_ex->Forward_gpu_native(conv2_4_ex_top_data);
			conv2_4_dw->Forward(cudnn_handle_, conv2_4_ex_top_data, conv2_4_dw_top_data);
			relu2_4_dw->Forward_gpu_native(conv2_4_dw_top_data);
			conv2_4_em->Forward(cudnn_handle_, conv2_4_dw_top_data, conv2_4_em_top_data);
			res2_4->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res2_3_top_data, conv2_4_em_top_data}, res2_4_top_data);
			conv3_ex->Forward(cudnn_handle_, res2_4_top_data, conv3_ex_top_data);
			relu3_ex->Forward_gpu_native(conv3_ex_top_data);
			conv3_dw->Forward(cudnn_handle_, conv3_ex_top_data, conv3_dw_top_data);
			relu3_dw->Forward_gpu_native(conv3_dw_top_data);
			conv3_em->Forward(cudnn_handle_, conv3_dw_top_data, conv3_em_top_data);
			conv3_1_ex->Forward(cudnn_handle_, conv3_em_top_data, conv3_1_ex_top_data);
			relu3_1_ex->Forward_gpu_native(conv3_1_ex_top_data);
			conv3_1_dw->Forward(cudnn_handle_, conv3_1_ex_top_data, conv3_1_dw_top_data);
			relu3_1_dw->Forward_gpu_native(conv3_1_dw_top_data);
			conv3_1_em->Forward(cudnn_handle_, conv3_1_dw_top_data, conv3_1_em_top_data);
			res3_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv3_em_top_data, conv3_1_em_top_data}, res3_1_top_data);
			conv3_2_ex->Forward(cudnn_handle_, res3_1_top_data, conv3_2_ex_top_data);
			relu3_2_ex->Forward_gpu_native(conv3_2_ex_top_data);
			conv3_2_dw->Forward(cudnn_handle_, conv3_2_ex_top_data, conv3_2_dw_top_data);
			relu3_2_dw->Forward_gpu_native(conv3_2_dw_top_data);
			conv3_2_em->Forward(cudnn_handle_, conv3_2_dw_top_data, conv3_2_em_top_data);
			res3_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_1_top_data, conv3_2_em_top_data}, res3_2_top_data);
			conv3_3_ex->Forward(cudnn_handle_, res3_2_top_data, conv3_3_ex_top_data);
			relu3_3_ex->Forward_gpu_native(conv3_3_ex_top_data);
			conv3_3_dw->Forward(cudnn_handle_, conv3_3_ex_top_data, conv3_3_dw_top_data);
			relu3_3_dw->Forward_gpu_native(conv3_3_dw_top_data);
			conv3_3_em->Forward(cudnn_handle_, conv3_3_dw_top_data, conv3_3_em_top_data);
			res3_3->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_2_top_data, conv3_3_em_top_data}, res3_3_top_data);
			conv3_4_ex->Forward(cudnn_handle_, res3_3_top_data, conv3_4_ex_top_data);
			relu3_4_ex->Forward_gpu_native(conv3_4_ex_top_data);
			conv3_4_dw->Forward(cudnn_handle_, conv3_4_ex_top_data, conv3_4_dw_top_data);
			relu3_4_dw->Forward_gpu_native(conv3_4_dw_top_data);
			conv3_4_em->Forward(cudnn_handle_, conv3_4_dw_top_data, conv3_4_em_top_data);
			res3_4->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_3_top_data, conv3_4_em_top_data}, res3_4_top_data);
			conv3_5_ex->Forward(cudnn_handle_, res3_4_top_data, conv3_5_ex_top_data);
			relu3_5_ex->Forward_gpu_native(conv3_5_ex_top_data);
			conv3_5_dw->Forward(cudnn_handle_, conv3_5_ex_top_data, conv3_5_dw_top_data);
			relu3_5_dw->Forward_gpu_native(conv3_5_dw_top_data);
			conv3_5_em->Forward(cudnn_handle_, conv3_5_dw_top_data, conv3_5_em_top_data);
			res3_5->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_4_top_data, conv3_5_em_top_data}, res3_5_top_data);
			conv3_6_ex->Forward(cudnn_handle_, res3_5_top_data, conv3_6_ex_top_data);
			relu3_6_ex->Forward_gpu_native(conv3_6_ex_top_data);
			conv3_6_dw->Forward(cudnn_handle_, conv3_6_ex_top_data, conv3_6_dw_top_data);
			relu3_6_dw->Forward_gpu_native(conv3_6_dw_top_data);
			conv3_6_em->Forward(cudnn_handle_, conv3_6_dw_top_data, conv3_6_em_top_data);
			res3_6->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res3_5_top_data, conv3_6_em_top_data}, res3_6_top_data);
			conv4_ex->Forward(cudnn_handle_, res3_6_top_data, conv4_ex_top_data);
			relu4_ex->Forward_gpu_native(conv4_ex_top_data);
			conv4_dw->Forward(cudnn_handle_, conv4_ex_top_data, conv4_dw_top_data);
			relu4_dw->Forward_gpu_native(conv4_dw_top_data);
			conv4_em->Forward(cudnn_handle_, conv4_dw_top_data, conv4_em_top_data);
			conv4_1_ex->Forward(cudnn_handle_, conv4_em_top_data, conv4_1_ex_top_data);
			relu4_1_ex->Forward_gpu_native(conv4_1_ex_top_data);
			conv4_1_dw->Forward(cudnn_handle_, conv4_1_ex_top_data, conv4_1_dw_top_data);
			relu4_1_dw->Forward_gpu_native(conv4_1_dw_top_data);
			conv4_1_em->Forward(cudnn_handle_, conv4_1_dw_top_data, conv4_1_em_top_data);
			res4_1->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{conv4_em_top_data, conv4_1_em_top_data}, res4_1_top_data);
			conv4_2_ex->Forward(cudnn_handle_, res4_1_top_data, conv4_2_ex_top_data);
			relu4_2_ex->Forward_gpu_native(conv4_2_ex_top_data);
			conv4_2_dw->Forward(cudnn_handle_, conv4_2_ex_top_data, conv4_2_dw_top_data);
			relu4_2_dw->Forward_gpu_native(conv4_2_dw_top_data);
			conv4_2_em->Forward(cudnn_handle_, conv4_2_dw_top_data, conv4_2_em_top_data);
			res4_2->Forward_gpu_native(cublas_handle_, std::vector<std::shared_ptr<memory::tensor<float>>>{res4_1_top_data, conv4_2_em_top_data}, res4_2_top_data);
			conv5_ex->Forward(cudnn_handle_, res4_2_top_data, conv5_ex_top_data);
			relu5_ex->Forward_gpu_native(conv5_ex_top_data);
			conv5_dw->Forward(cudnn_handle_, conv5_ex_top_data, conv5_dw_top_data);
			fc5->Forward_gpu_native(cublas_handle_, conv5_dw_top_data, fc5_top_data);
		}
#endif
#endif

		std::vector<std::vector<float> > Unicorn_mobile_mask::Forward(const float* input_data, unsigned num, int order)
		{
			if (num <= 0)
			{
				LOG(FATAL) << "no human face information!!!";
				return std::vector<std::vector<float> >();
			}

			std::vector<std::vector<float> > feature;

			if (order == 0)//NCHW
			{
				tensor_float_data.reset(new memory::tensor<float>(std::vector<int>{(int)num, 3, 128, 128}, device_, memory::NCHW));
			}
			else//NHWC
			{
				tensor_float_data.reset(new memory::tensor<float>(std::vector<int>{(int)num, 128, 128, 3}, device_, memory::NHWC));
			}

			float means[3] = { 104.0f, 117.0f, 124.0f };
			float var = 0.0078125f;

			if (device_ < 0)
			{
				float* tensor_data = tensor_float_data->mutable_cpu_data();
				memcpy(tensor_data, input_data, num * 3 * 128 * 128 * sizeof(float));

				//add mask
				if (order == 0)
				{
					for (int ch = 0; ch < 3; ch++)
					{
						memset(tensor_data + ch * 128 * 128 + 64 * 128, 0, 64 * 128 * sizeof(float));
					}
				}
				else
				{
					memset(tensor_data + 3 * 64 * 128, 0, 3 * 64 * 128 * sizeof(float));
				}

				tensor_operation_cpu::preprocess_tensors_cpu(tensor_float_data, tensor_float_data, means, var);

				std::shared_ptr<memory::tensor<float>> src_tensor = tensor_float_data;
#ifdef __ARM_NEON
				if (order == 1)
					tensor_operation_cpu::nhwc2nchw_cpu(tensor_float_data, src_tensor);
#endif

				Forward_cpu(src_tensor);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
			}
			else
			{
#ifdef USE_CUDA
				float* tensor_data = tensor_float_data->mutable_gpu_data();
				CUDA_CHECK(cudaMemcpy(tensor_data, input_data, num * 3 * 128 * 128 * sizeof(float), cudaMemcpyDefault));

				//add mask
				if (order == 0)
				{
					for (int ch = 0; ch < 3; ch++)
					{
						cudaMemset(tensor_data + ch * 128 * 128 + 64 * 128, 0, 64 * 128 * sizeof(float));
					}
				}
				else
				{
					cudaMemset(tensor_data + 3 * 64 * 128, 0, 3 * 64 * 128 * sizeof(float));
				}

				tensor_operation_gpu::preprocess_tensors_gpu(tensor_float_data, tensor_float_data, means, var);
#ifdef USE_CUDNN
				Forward_gpu_cudnn(tensor_float_data);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
#endif
				Forward_gpu_native(tensor_float_data);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
#else
				NO_GPU;
				return feature;
#endif
			}
		}

		std::vector<std::vector<float> > Unicorn_mobile_mask::Forward(const unsigned char* input_data, unsigned num, int order)
		{
			if (num <= 0)
			{
				LOG(FATAL) << "no human face information!!!";
				return std::vector<std::vector<float> >();
			}

			std::vector<std::vector<float> > feature;

			if (order == 0)//NCHW
			{
				tensor_unsigned_char_data.reset(new memory::tensor<unsigned char>(std::vector<int>{(int)num, 3, 128, 128}, device_, memory::NCHW));
				tensor_float_data.reset(new memory::tensor<float>(std::vector<int>{(int)num, 3, 128, 128}, device_, memory::NCHW));
			}
			else//NHWC
			{
				tensor_unsigned_char_data.reset(new memory::tensor<unsigned char>(std::vector<int>{(int)num, 128, 128, 3}, device_, memory::NHWC));
				tensor_float_data.reset(new memory::tensor<float>(std::vector<int>{(int)num, 128, 128, 3}, device_, memory::NHWC));
			}

			float means[3] = { 104.0f, 117.0f, 124.0f };
			float var = 0.0078125f;

			if (device_ < 0)
			{
				unsigned char* tensor_data = tensor_unsigned_char_data->mutable_cpu_data();
				memcpy(tensor_data, input_data, num * 3 * 128 * 128 * sizeof(unsigned char));

				//add mask
				if (order == 0)
				{
					for (int ch = 0; ch < 3; ch++)
					{
						memset(tensor_data + ch * 128 * 128 + 64 * 128, 0, 64 * 128 * sizeof(unsigned char));
					}
				}
				else
				{
					memset(tensor_data + 3 * 64 * 128, 0, 3 * 64 * 128 * sizeof(unsigned char));
				}

				tensor_operation_cpu::preprocess_tensors_cpu(tensor_unsigned_char_data, tensor_float_data, means, var);
				Forward_cpu(tensor_float_data);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
			}
			else
			{
#ifdef USE_CUDA
				unsigned char* tensor_data = tensor_unsigned_char_data->mutable_gpu_data();
				CUDA_CHECK(cudaMemcpy(tensor_data, input_data, num * 3 * 128 * 128 * sizeof(unsigned char), cudaMemcpyDefault));

				//add mask
				if (order == 0)
				{
					for (int ch = 0; ch < 3; ch++)
					{
						cudaMemset(tensor_data + ch * 128 * 128 + 64 * 128, 0, 64 * 128 * sizeof(unsigned char));
					}
				}
				else
				{
					cudaMemset(tensor_data + 3 * 64 * 128, 0, 3 * 64 * 128 * sizeof(unsigned char));
				}

				tensor_operation_gpu::preprocess_tensors_gpu(tensor_unsigned_char_data, tensor_float_data, means, var);
#ifdef USE_CUDNN
				Forward_gpu_cudnn(tensor_float_data);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
#endif
				Forward_gpu_native(tensor_float_data);
				for (size_t i = 0; i < num; i++)
				{
					std::vector<float> temp(128);
					std::memcpy(temp.data(), get_fc5()->cpu_data() + i * 128, 128 * sizeof(float));
					feature.push_back(temp);
				}
				return feature;
#else
				NO_GPU;
				return feature;
#endif
			}
		}
	}
}
#pragma once
#ifndef _OPERATION_RGB2GRAY_HPP_
#define _OPERATION_RGB2GRAY_HPP_
#include <memory>
#include <Primitives/tensor.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/simd_types.hpp>

namespace glasssix
{
	namespace excalibur
	{
		/// <summary>
		/// convert 3 channels image to 1 channel
		/// </summary>
		/// <param name="src">original memory::tensor</param>
		/// <param name="dst">new memory::tensor</param>
		static void rgb2gray_cpu(const std::shared_ptr<memory::tensor<unsigned char>> &src, std::shared_ptr<memory::tensor<unsigned char>>& dst)
		{
			if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}

			int num = src->num();
			int channels = src->channels();
			int height = src->height();
			int width = src->width();
			int offset = height * width;
			int num_offset = channels * height * width;

			if (!(channels == 3 || channels == 4))
			{
				LOG(ERROR) << "Incorrect input channel.";
				return;
			}

			std::shared_ptr<memory::tensor<unsigned char>> dst_temp;

			if (src->order() == memory::NCHW)
			{
				dst_temp.reset(new memory::tensor<unsigned char>(std::vector<int>{num, 1, height, width}, src->device(), src->order(), src->allocator()));
				const unsigned char* src_data = src->cpu_data();
				unsigned char* dst_data = dst_temp->mutable_cpu_data();

#if SIMD_TYPE >= SIMDTYPE_SSE

				//B / G / R
				mm_type factor_float32[] = { mm_set1_ps(0.114f), mm_set1_ps(0.587f), mm_set1_ps(0.299f) };
				std::shared_ptr<memory::tensor<float>> temp_float_tensor = std::make_shared<memory::tensor<float>>(mm_align_size);
				float *temp_float_data = temp_float_tensor->mutable_cpu_data();

				__m128i temp_uint8_B, temp_uint8_G, temp_uint8_R;
				mm_type temp_float32_B, temp_float32_G, temp_float32_R, temp_float32_sum;
				mm_typei temp_int32_B, temp_int32_G, temp_int32_R;

				int circle_num = offset / mm_align_size;
				int index = 0;

				for (int n = 0; n < num; n++)
				{
					int n_offset = n * num_offset;
					index = 0;

					for (; index < circle_num; index++)
					{
						temp_uint8_B = _mm_loadl_epi64((__m128i const*)(src_data + n_offset + index * mm_align_size));
						temp_uint8_G = _mm_loadl_epi64((__m128i const*)(src_data + n_offset + offset + index * mm_align_size));
						temp_uint8_R = _mm_loadl_epi64((__m128i const*)(src_data + n_offset + 2 * offset + index * mm_align_size));
						temp_int32_B = mm_cvtepu8_epi32(temp_uint8_B);
						temp_int32_G = mm_cvtepu8_epi32(temp_uint8_G);
						temp_int32_R = mm_cvtepu8_epi32(temp_uint8_R);
						temp_float32_B = mm_cvtepi32_ps(temp_int32_B);
						temp_float32_G = mm_cvtepi32_ps(temp_int32_G);
						temp_float32_R = mm_cvtepi32_ps(temp_int32_R);
						temp_float32_B = mm_mul_ps(temp_float32_B, factor_float32[0]);
						temp_float32_G = mm_mul_ps(temp_float32_G, factor_float32[1]);
						temp_float32_R = mm_mul_ps(temp_float32_R, factor_float32[2]);
						temp_float32_sum = mm_add_ps(temp_float32_B, temp_float32_G);
						temp_float32_sum = mm_add_ps(temp_float32_sum, temp_float32_R);
						mm_store_ps(temp_float_data, temp_float32_sum);
						for (int i = 0; i < mm_align_size; i++)
						{
							dst_data[n * offset + index * mm_align_size + i] = (unsigned char)(temp_float_data[i]);
						}
					}

					for (index *= mm_align_size; index < offset; index++)
					{
						//pixel order in opencv: B / G / R
						//convert formula: gray = 0.114 * B + 0.587 * G + 0.299 * R
						dst_data[n * offset + index] = (unsigned char)(src_data[n_offset + index] * 0.114f +
																	   src_data[n_offset + offset * 1 + index] * 0.587f +
																	   src_data[n_offset + offset * 2 + index] * 0.299f);
					}
				}
#else
				for (int n = 0; n < num; n++)
				{
					int n_offset = n * num_offset;
					for (int index = 0; index < offset; index++)
					{
						//pixel order in opencv: B / G / R
						//convert formula: gray = 0.114 * B + 0.587 * G + 0.299 * R
						dst_data[n * offset + index] = (unsigned char)(src_data[n_offset + index] * 0.114f +
																	   src_data[n_offset + offset * 1 + index] * 0.587f +
																	   src_data[n_offset + offset * 2 + index] * 0.299f);
					}
				}
#endif //!SIMD_TYPE >= SIMDTYPE_SSE

			}
			else if (src->order() == memory::NHWC)
			{
				dst_temp.reset(new memory::tensor<unsigned char>(std::vector<int>{num, height, width, 1}, src->device(), src->order(), src->allocator()));
				const unsigned char* src_data = src->cpu_data();
				unsigned char* dst_data = dst_temp->mutable_cpu_data();

				for (int n = 0; n < num; n++)
				{
					for (int row = 0; row < height; ++row)
					{
						int dst_pos1 = row * width;
						int src_pos1 = row * width * channels;
						for (int col = 0; col < width; ++col)
						{
							int dst_pos2 = dst_pos1 + col;
							int src_pos2 = src_pos1 + col * channels;
							//pixel order in opencv: B / G / R
							//convert formula: gray = 0.114 * B + 0.587 * G + 0.299 * R
							dst_data[n * offset + dst_pos2] = (unsigned char)(src_data[n * num_offset + src_pos2] * 0.114f +
																			  src_data[n * num_offset + src_pos2 + 1] * 0.587f +
																			  src_data[n * num_offset + src_pos2 + 2] * 0.299f);
						}
					}
				}
			}
			else
			{
				NOT_IMPLEMENTED;
			}

			dst = std::make_shared<memory::tensor<unsigned char>>(dst_temp->clone());
		}
	}
}
#endif
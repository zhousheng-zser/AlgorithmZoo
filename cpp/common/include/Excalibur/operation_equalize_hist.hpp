#pragma once
#ifndef _OPERATION_EQUALIZE_HIST_HPP_
#define _OPERATION_EQUALIZE_HIST_HPP_
#include <memory>
#include <Primitives/logger.hpp>

namespace glasssix
{
	namespace excalibur
	{
		/// <summary>
		/// equalize histogram, gray image required
		/// </summary>
		/// <param name="src">original memory::tensor</param>
		/// <param name="dst">new memory::tensor</param>
		template <typename Dtype>
		static void equalize_hist_cpu(const std::shared_ptr<memory::tensor<Dtype>> &src, std::shared_ptr<memory::tensor<Dtype>>& dst)
		{
			if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}

			int num = src->num();
			CHECK_EQ(src->channels(), 1);
			int height = src->height();
			int width = src->width();
			int offset = height * width;

			std::shared_ptr<memory::tensor<Dtype>> dst_temp;
			dst_temp.reset(new memory::tensor<Dtype>(src->data_shape(), src->device(), src->order()));
			Dtype* dst_data = dst_temp->mutable_cpu_data();
			const Dtype* src_data = src->cpu_data();

			for (int n = 0; n < num; n++)
			{
				int gray_value[256] = { 0 };
				float probability_distribution[256] = { 0 };
				float accumulate_probability_distribution[256] = { 0 };
				int normalized_gray_value[256] = { 0 };

				//Count the number of pixels in each grayscale
				for (int i = 0; i < offset; i++)
				{
					int value = static_cast<unsigned char>(src_data[n * offset + i]);
					gray_value[value]++;
				}

				for (int i = 0; i < 256; i++)
				{
					probability_distribution[i] = static_cast<float>(gray_value[i]) / offset;

					if (i > 0)
					{
						accumulate_probability_distribution[i] = accumulate_probability_distribution[i - 1] + probability_distribution[i];
					}
					else
					{
						accumulate_probability_distribution[0] = probability_distribution[0];
					}

					normalized_gray_value[i] = static_cast<unsigned char>(255 * accumulate_probability_distribution[i] + 0.5);
				}

				for (int i = 0; i < offset; i++)
				{
					dst_data[n * offset + i] = Dtype(normalized_gray_value[static_cast<unsigned char>(src_data[n * offset + i])]);
				}
			}

			dst = std::make_shared<memory::tensor<Dtype>>(dst_temp->clone());
		}
	}
}
#endif
#pragma once
#ifndef _OPERATION_CUT_BORDER_HPP_
#define _OPERATION_CUT_BORDER_HPP_
#include <memory>
#include <Primitives/logger.hpp>

namespace glasssix
{
	namespace excalibur
	{
		/// <summary>
		/// expand border
		/// </summary>
		/// <param name="src">original memory::tensor</param>
		/// <param name="dst">new memory::tensor</param>
		/// <param name="top">pixels to expand at top of image</param>
		/// <param name="bottom">pixels to expand at bottom of image</param>
		/// <param name="left">pixels to expand at left of image</param>
		/// <param name="right">pixels to expand at right of image</param>

		template <typename Dtype>
		static void cut_border_cpu(const std::shared_ptr<memory::tensor<Dtype>>& src,
			std::shared_ptr<memory::tensor<Dtype>>& dst, int top, int bottom, int left, int right)
		{
			int num = src->num();
			int channels = src->channels();
			int height = src->height();
			int width = src->width();
			int src_offset = height * width;
			int src_num_offset = channels * height * width;

			int dst_height = height - top - bottom;
			int dst_width = width - left - right;
			int dst_offset = dst_height * dst_width;
			int dst_num_offset = channels * dst_height * dst_width;
			if (dst_height == height && dst_width == width)
			{
				dst = std::make_shared<memory::tensor<Dtype>>(src->clone());
				return;
			}

			if (dst_height <= 0 || dst_width <= 0)
			{
				LOG(ERROR) << "Illegal input size.";
				return;
			}

			std::shared_ptr<memory::tensor<Dtype>> dst_temp;
			if (src->order() == memory::NCHW)
			{
				dst_temp.reset(new memory::tensor<Dtype>(std::vector<int>{num, channels, dst_height, dst_width}, src->device(), src->order(), src->allocator()));
				Dtype* dst_data = dst_temp->mutable_cpu_data();
				const Dtype* src_data = src->cpu_data();

				for (int n = 0; n < num; n++)
				{
					int src_n_offset = n * src_num_offset;
					int dst_n_offset = n * dst_num_offset;

					for (int ch = 0; ch < channels; ++ch)
					{
						int src_channel_offset = ch * src_offset;
						int dst_channel_offset = ch * dst_offset;

						for (int row = 0; row < dst_height; ++row)
						{
							int src_index = src_channel_offset + (row + top) * width + left;
							int dst_index = dst_channel_offset + row * dst_width;
							memcpy(dst_data + dst_n_offset + dst_index, src_data + src_n_offset + src_index, dst_width * sizeof(Dtype));
						}
					}
				}
			}
			else
			{
				NOT_IMPLEMENTED;
			}

			dst = std::make_shared<memory::tensor<Dtype>>(dst_temp->clone());
		}
	}
}
#endif // !_OPERATION_CUT_BORDER_HPP_

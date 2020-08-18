#pragma once
#ifndef _OPERATION_MERGE_CHANNEL_HPP_
#define _OPERATION_MERGE_CHANNEL_HPP_
#include <memory>
#include <Primitives/logger.hpp>

namespace glasssix
{
	namespace excalibur
	{
		/// <summary>
		/// merge 3 single-channel images together, to create one 3-channel image 
		/// </summary>
		/// <param name="src_vector">array of tensors, each memory::tensor should share the same height/width/device/order</param>
		/// <param name="dst">new memory::tensor</param>
		template <typename Dtype>
		static void merge_channel_cpu(const std::vector<std::shared_ptr<memory::tensor<Dtype>>> &src_vector, std::shared_ptr<memory::tensor<Dtype>> &dst)
		{
			CHECK_EQ(src_vector.size(), 3);
			int height, width, device;
			memory::orderType order;
			for (int i = 0; i < src_vector.size(); ++i)
			{
				CHECK_EQ(src_vector.at(i)->channels(), 1);
				if (i == 0)
				{
					height = src_vector.at(i)->height();
					width = src_vector.at(i)->width();
					device = src_vector.at(i)->device();
					order = src_vector.at(i)->order();

					if (device >= 0)
					{
						LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
						return;
					}
				}
				else
				{
					if (height != src_vector.at(i)->height() ||
						width != src_vector.at(i)->width() ||
						device != src_vector.at(i)->device() ||
						order != src_vector.at(i)->order())
					{
						LOG(WARNING) << "the element of vector<mat> should have the exact same height/width/device/type.";
						return;
					}
				}
			}

			if (order == memory::NCHW)
			{
				dst.reset(new memory::tensor<Dtype>(std::vector<int>{1, 3, height, width}, device, order));
			}
			else if (order == memory::NHWC)
			{
				dst.reset(new memory::tensor<Dtype>(std::vector<int>{1, height, width, 3}, device, order));
			}
			else
			{
				NOT_IMPLEMENTED;
			}

			Dtype* dst_data = dst->mutable_cpu_data();
			int offset = height * width;

			for (int i = 0; i < src_vector.size(); ++i)
			{
				const Dtype* temp_data = src_vector.at(i)->cpu_data();
				std::memcpy((void*)(dst_data + i * offset), (void*)(temp_data), offset * sizeof(Dtype));
			}
		}
	}
}
#endif
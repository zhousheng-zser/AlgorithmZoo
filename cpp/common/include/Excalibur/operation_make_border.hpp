#pragma once
#ifndef _OPERATION_MAKE_BORDER_HPP_
#define _OPERATION_MAKE_BORDER_HPP_
#include <memory>
#include <Primitives/tensor.hpp>
namespace glasssix
{
	namespace excalibur
	{
		enum border_type { border_constant, border_replicate };

		/// <summary>
		/// expand border
		/// </summary>
		/// <param name="src">original memory::tensor</param>
		/// <param name="dst">new memory::tensor</param>
		/// <param name="top">pixels to expand at top of image</param>
		/// <param name="bottom">pixels to expand at bottom of image</param>
		/// <param name="left">pixels to expand at left of image</param>
		/// <param name="right">pixels to expand at right of image</param>
		/// <param name="type">borderType: Border_Constant(default, use constant pixel value(fill_pixel_value) to fill in new blank area) / Border_Replicate(replicate neighboring pixel to fill in new blank area)</param>
		/// <param name="fill_pixel_value">validate when borderType is Border_Constant, zero by default</param>
		template <typename Dtype>
		static void make_border(const std::shared_ptr<memory::tensor<Dtype>> &src, std::shared_ptr<memory::tensor<Dtype>>& dst,
			int top, int bottom, int left, int right, border_type type = border_constant, Dtype fill_pixel_value = 0)
		{
			/*if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}*/

			int num = src->num();
			int channels = src->channels();
			int height = src->height();
			int width = src->width();
			int src_offset = height * width;
			int src_num_offset = channels * height * width;

			int dst_height = height + top + bottom;
			int dst_width = width + left + right;
			int dst_offset = dst_height * dst_width;
			int dst_num_offset = channels * dst_height * dst_width;

			if (top < 0 || bottom < 0 || left < 0 || right < 0)
			{
				LOG(ERROR) << "top, bottom, left, right: should all be non-negtive.";
				return;
			}

			if (dst_height == height && dst_width == width)
			{
				dst = std::make_shared<memory::tensor<Dtype>>(src->clone());
				return;
			}

			std::shared_ptr<memory::tensor<Dtype>> dst_temp;
			if (src->order() == memory::NCHW)
			{
				dst_temp.reset(new memory::tensor<Dtype>(std::vector<int>{num, channels, dst_height, dst_width}, src->device(), src->order(), src->allocator()));
				Dtype* dst_data = dst_temp->mutable_cpu_data();
				const Dtype* src_data = src->cpu_data();

				if (type == border_constant)
				{
					for (int n = 0; n < num; n++)
					{
						int src_n_offset = n * src_num_offset;
						int dst_n_offset = n * dst_num_offset;
#ifdef _OPENMP
#pragma omp parallel for num_threads(2)
#endif
						for (int ch = 0; ch < channels; ++ch)
						{
							int src_channel_offset = ch * src_offset;
							int dst_channel_offset = ch * dst_offset;
							
							//top
							for (int row = 0; row < top; row++)
							{
								int dst_index = dst_channel_offset + row * dst_width;
								for (int col = 0; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = fill_pixel_value;
								}
							}

							//center
							for (int row = top; row < top + height; ++row)
							{
								int src_index = src_channel_offset + (row - top) * width;
								int dst_index = dst_channel_offset + row * dst_width;

								for (int col = 0; col < left; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = fill_pixel_value;
								}

								memcpy(dst_data + dst_n_offset + dst_index + left, src_data + src_n_offset + src_index, width * sizeof(Dtype));

								for (int col = left + width; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = fill_pixel_value;
								}
							}

							//bottom
							for (int row = top + height; row < dst_height; row++)
							{
								int dst_index = dst_channel_offset + row * dst_width;
								for (int col = 0; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = fill_pixel_value;
								}
							}
							
						}
					}
				}
				else if (type == border_replicate)
				{
					for (int n = 0; n < num; n++)
					{
						int src_n_offset = n * src_num_offset;
						int dst_n_offset = n * dst_num_offset;
						for (int ch = 0; ch < channels; ++ch)
						{
							int src_channel_offset = ch * src_offset;
							int dst_channel_offset = ch * dst_offset;

							//top
							for (int row = 0; row < top; ++row)
							{
								int dst_index = dst_channel_offset + row * dst_width;

								//left
								for (int col = 0; col < left; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_channel_offset];
								}

								//center
								memcpy(dst_data + dst_n_offset + dst_index + left, src_data + src_n_offset + src_channel_offset, width * sizeof(Dtype));

								//right
								for (int col = left + width; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_channel_offset + width - 1];
								}
							}

							//center
							for (int row = top; row < top + height; ++row)
							{
								int src_index = src_channel_offset + (row - top) * width;
								int dst_index = dst_channel_offset + row * dst_width;

								//left
								for (int col = 0; col < left; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_index];
								}

								//center
								memcpy(dst_data + dst_n_offset + dst_index + left, src_data + src_n_offset + src_index, width * sizeof(Dtype));

								//right
								for (int col = left + width; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_index + width - 1];
								}
							}

							//bottom
							for (int row = top + height; row < dst_height; ++row)
							{
								int src_index = src_channel_offset + (height - 1) * width;
								int dst_index = dst_channel_offset + row * dst_width;

								//left
								for (int col = 0; col < left; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_index];
								}

								//center
								memcpy(dst_data + dst_n_offset + dst_index + left, src_data + src_n_offset + src_index, width * sizeof(Dtype));

								//right
								for (int col = left + width; col < dst_width; col++)
								{
									dst_data[dst_n_offset + dst_index + col] = src_data[src_n_offset + src_index + width - 1];
								}
							}
						}
					}
				}
				else
				{
					LOG(ERROR) << "Un-support border type.";
					return;
				}
			}
			else if (src->order() == memory::NHWC)
			{
				dst_temp.reset(new memory::tensor<Dtype>(std::vector<int>{num, dst_height, dst_width, channels}, src->device(), src->order()));
				Dtype* dst_data = dst_temp->mutable_cpu_data();
				const Dtype* src_data = src->cpu_data();

				if (type == border_constant)
				{
					for (int n = 0; n < num; n++)
					{
						int src_n_offset = n * src_num_offset;
						int dst_n_offset = n * dst_num_offset;

						//top
						for (int row = 0; row < top; row++)
						{
							int dst_index1 = row * dst_width * channels;
							for (int col = 0; col < dst_width; col++)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ch++)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = fill_pixel_value;
								}
							}
						}

						//center
						for (int row = top; row < top + height; ++row)
						{
							int src_index = (row - top) * width * channels;

							//left
							int dst_index1 = row * dst_width * channels;
							for (int col = 0; col < left; col++)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ch++)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = fill_pixel_value;
								}
							}

							//center
							memcpy(dst_data + dst_n_offset + dst_index1 + left * channels, src_data + src_n_offset + src_index, width * channels * sizeof(Dtype));

							//right
							for (int col = left + width; col < dst_width; col++)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ch++)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = fill_pixel_value;
								}
							}
						}

						//bottom
						for (int row = top + height; row < dst_height; row++)
						{
							int dst_index1 = row * dst_width * channels;
							for (int col = 0; col < dst_width; col++)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ch++)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = fill_pixel_value;
								}
							}
						}
					}
				}
				else if (type == border_replicate)
				{
					for (int n = 0; n < num; n++)
					{
						int src_n_offset = n * src_num_offset;
						int dst_n_offset = n * dst_num_offset;

						//top
						for (int row = 0; row < top; ++row)
						{
							int dst_index1 = row * dst_width * channels;

							//left
							for (int col = 0; col < left; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + ch];
								}
							}

							//center
							memcpy(dst_data + dst_n_offset + dst_index1 + left * channels, src_data + src_n_offset, width * channels * sizeof(Dtype));

							//right
							for (int col = left + width; col < dst_width; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								int src_index = (width - 1) * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + src_index + ch];
								}
							}
						}


						//center
						for (int row = top; row < top + height; ++row)
						{
							int src_index1 = (row - top) * width * channels;
							int dst_index1 = row * dst_width * channels;

							//left
							for (int col = 0; col < left; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + src_index1 + ch];
								}
							}

							//center
							memcpy(dst_data + dst_n_offset + dst_index1 + left * channels, src_data + src_n_offset + src_index1, width * channels * sizeof(Dtype));

							//right
							int src_index2 = src_index1 + (width - 1) * channels;
							for (int col = left + width; col < dst_width; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + src_index2 + ch];
								}
							}
						}


						//bottom
						for (int row = top + height; row < dst_height; ++row)
						{
							int dst_index1 = row * dst_width * channels;
							int src_index1 = (height - 1) * width * channels;

							//left
							for (int col = 0; col < left; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + src_index1 + ch];
								}
							}

							//center
							memcpy(dst_data + dst_n_offset + dst_index1 + left * channels, src_data + src_n_offset + src_index1, width * channels * sizeof(Dtype));

							//right
							int src_index2 = src_index1 + (width - 1) * channels;
							for (int col = left + width; col < dst_width; ++col)
							{
								int dst_index2 = dst_index1 + col * channels;
								for (int ch = 0; ch < channels; ++ch)
								{
									dst_data[dst_n_offset + dst_index2 + ch] = src_data[src_n_offset + src_index2 + ch];
								}
							}
						}
					}
				}
				else
				{
					LOG(ERROR) << "Un-support border type.";
					return;
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
#endif // !_OPERATION_MAKE_BORDER_HPP_

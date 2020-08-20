#pragma once
#ifndef _OPERATION_RESIZE_HPP_
#define _OPERATION_RESIZE_HPP_
#include <memory>
#include <algorithm>
#include <Primitives/logger.hpp>

namespace glasssix
{
	namespace excalibur
	{

#define PI 3.141592f
#define ICV_WARP_SHIFT          10
#define ICV_WARP_SHIFT2         15
#define ICV_SHIFT_DIFF          (ICV_WARP_SHIFT2-ICV_WARP_SHIFT)
#define ICV_WARP_MASK           ((1 << ICV_WARP_SHIFT) - 1)
#define ICV_WARP_MUL_ONE_8U(x)  ((x) << ICV_WARP_SHIFT)
#define ICV_WARP_DESCALE_8U(x)  CV_DESCALE((x), ICV_WARP_SHIFT*2)
#define CV_SWAP(a,b,t)          ((t) = (a), (a) = (b), (b) = (t))
#define CV_DESCALE(x,n)         (((x) + (1 << ((n)-1))) >> (n))

		typedef struct CvResizeAlpha
		{
			int idx;
			int ialpha;

		}CvResizeAlpha;

		inline static int icvResize_Bilinear_8u_C1(const unsigned char* src, int srcstep, int swidth, int sheight,
			unsigned char* dst, int dststep, int dwidth, int dheight,
			int xmax,
			const CvResizeAlpha* xofs,
			const CvResizeAlpha* yofs,
			int* buf0, int* buf1)
		{
			int prev_sy0 = -1, prev_sy1 = -1;
			int k, dx, dy;

			srcstep /= sizeof(src[0]);
			dststep /= sizeof(dst[0]);

			for (dy = 0; dy < dheight; dy++, dst += dststep)
			{
				int fy = yofs[dy].ialpha, * swap_t;
				int sy0 = yofs[dy].idx, sy1 = sy0 + (fy > 0 && sy0 < sheight - 1);

				if (sy0 == prev_sy0 && sy1 == prev_sy1)
					k = 2;
				else if (sy0 == prev_sy1)
				{
					CV_SWAP(buf0, buf1, swap_t);
					k = 1;
				}
				else
					k = 0;

				for (; k < 2; k++)
				{
					int* _buf = k == 0 ? buf0 : buf1;
					const unsigned char* _src;
					int sy = k == 0 ? sy0 : sy1;
					if (k == 1 && sy1 == sy0)
					{
						memcpy(buf1, buf0, dwidth * sizeof(buf0[0]));
						continue;
					}

					_src = src + sy * srcstep;
					for (dx = 0; dx < xmax; dx++)
					{
						int sx = xofs[dx].idx;
						int fx = xofs[dx].ialpha;
						int t = _src[sx];
						_buf[dx] = ICV_WARP_MUL_ONE_8U(t) + fx * (_src[sx + 1] - t);
					}

					for (; dx < dwidth; dx++)
						_buf[dx] = ICV_WARP_MUL_ONE_8U(_src[xofs[dx].idx]);
				}

				prev_sy0 = sy0;
				prev_sy1 = sy1;

				if (sy0 == sy1)
					for (dx = 0; dx < dwidth; dx++)
						dst[dx] = (unsigned char)ICV_WARP_DESCALE_8U(ICV_WARP_MUL_ONE_8U(buf0[dx]));
				else
					for (dx = 0; dx < dwidth; dx++)
						dst[dx] = (unsigned char)ICV_WARP_DESCALE_8U(ICV_WARP_MUL_ONE_8U(buf0[dx]) +
							fy * (buf1[dx] - buf0[dx]));
			}

			return 1;
		}

		enum interpolationType { Nearest, Bilinear, Cubic };

		/// <summary>
		/// resize image data
		/// </summary>
		/// <param name="src">memory::tensor of image with original size(height and width)</param>
		/// <param name="dst">memory::tensor of image with new size</param>
		/// <param name="dst_height">new height</param>
		/// <param name="dst_width">new width</param>
		/// <param name="type">interpolationType: Nearest / Bilinear(default)</param>
		template <typename Dtype>
		static void resize_cpu(const std::shared_ptr<memory::tensor<Dtype>> &src, std::shared_ptr<memory::tensor<Dtype>>& dst,
			int dst_height, int dst_width, interpolationType type = Bilinear)
		{
			/*if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}*/

			if (dst_height * dst_width <= 0)
			{
				LOG(ERROR) << "Illegal input size.";
				return;
			}
			
			int num = src->num();
			int channels = src->channels();
			int height = src->height();
			int width = src->width();
			int src_offset = height * width;
			int dst_offset = dst_height * dst_width;
			int src_num_offset = channels * height * width;
			int dst_num_offset = channels * dst_height * dst_width;
			unsigned maxIndex = height * width * channels - 1;

			if (dst_width == width && dst_height == height)
			{
				dst = std::make_shared<memory::tensor<Dtype>>(src->clone());
				return;
			}

			std::shared_ptr<memory::tensor<Dtype>> dst_temp;
			if (src->order() == memory::NCHW)
			{					
				dst_temp.reset(new memory::tensor<Dtype>(std::vector<int>{num, channels, dst_height, dst_width}, src->device(), src->order()));
				Dtype* dst_data = dst_temp->mutable_cpu_data();
				const Dtype* src_data = src->cpu_data();

				auto name = typeid(Dtype).name();

#ifdef _MSC_VER
				if (std::string("unsigned char") == std::string(name))
#else
				if (std::string("h") == std::string(name))
#endif
				{
					void* temp_buf = 0;
					int scale_x, scale_y;
					int sx, sy, dx, dy;
					int xmax = dst_width, buf_size;
					int *buf0, *buf1;
					CvResizeAlpha *xofs, *yofs;
					int fx_1024x, fy_1024x;

					scale_x = ((width << ICV_WARP_SHIFT2) + dst_width / 2) / dst_width;
					scale_y = ((height << ICV_WARP_SHIFT2) + dst_height / 2) / dst_height;

					buf_size = dst_width * 2 * sizeof(int) + (dst_width + dst_height) * sizeof(CvResizeAlpha);
					temp_buf = buf0 = (int*)malloc(buf_size);
					buf1 = buf0 + dst_width;
					xofs = (CvResizeAlpha*)(buf1 + dst_width);
					yofs = xofs + dst_width;

					for (dx = 0; dx < dst_width; dx++)
					{
						fx_1024x = ((dx * 2 + 1)*scale_x - (1 << ICV_WARP_SHIFT2)) / 2;
						sx = (fx_1024x >> ICV_WARP_SHIFT2);
						fx_1024x = ((fx_1024x - (sx << ICV_WARP_SHIFT2)) >> ICV_SHIFT_DIFF);

						if (sx < 0)
						{
							sx = 0;
							fx_1024x = 0;
						}

						if (sx >= width - 1)
						{
							fx_1024x = 0;
							sx = width - 1;

							if (xmax >= dst_width)
							{
								xmax = dx;
							}
						}

						xofs[dx].idx = sx;
						xofs[dx].ialpha = fx_1024x;
					}

					for (dy = 0; dy < dst_height; dy++)
					{
						fy_1024x = ((dy * 2 + 1)*scale_y - (1 << ICV_WARP_SHIFT2)) / 2;
						sy = (fy_1024x >> ICV_WARP_SHIFT2);
						fy_1024x = ((fy_1024x - (sy << ICV_WARP_SHIFT2)) >> ICV_SHIFT_DIFF);

						if (sy < 0)
						{
							sy = 0;
							fy_1024x = 0;
						}

						yofs[dy].idx = sy;
						yofs[dy].ialpha = fy_1024x;
					}

					for (int n = 0; n < num; n++)
					{
						int src_n_offset = n * src_num_offset;
						int dst_n_offset = n * dst_num_offset;

						for (int ch = 0; ch < channels; ++ch)
						{
							int src_channel_offset = ch * src_offset;
							int dst_channel_offset = ch * dst_offset;

							icvResize_Bilinear_8u_C1((const unsigned char*)&src_data[src_n_offset + src_channel_offset], width * sizeof(unsigned char), width, height, (unsigned char*)&dst_data[dst_n_offset + dst_channel_offset],
								dst_width * sizeof(unsigned char), dst_width, dst_height, xmax, xofs, yofs, buf0, buf1);
						}
					}

					free(temp_buf);
				}
				else
				{
					float width_ratio = (float)width / dst_width;
					float height_ratio = (float)height / dst_height;
					float beta = 0.5f;

#ifdef _OPENMP
#pragma omp parallel for
#endif
					for (int row = 0; row < dst_height; ++row)
					{
						float yf = row * height_ratio + beta;
						int y = (int)yf;
						float ydiff = yf - y;

						int src_pos1 = y * width;
						int dst_pos1 = row * dst_width;

						for (int col = 0; col < dst_width; ++col)
						{
							float xf = col * width_ratio + beta;
							int x = (int)xf;
							float xdiff = xf - x;

							int src_pos2 = src_pos1 + x;
							int dst_pos2 = dst_pos1 + col;

							for (int n = 0; n < num; n++)
							{
								int src_n_offset = n * src_num_offset;
								int dst_n_offset = n * dst_num_offset;

								for (int ch = 0; ch < channels; ++ch)
								{
									int src_pos3 = src_pos2 + ch * src_offset;
									int dst_pos3 = dst_pos2 + ch * dst_offset;

									if (type == Nearest)
									{
										dst_data[dst_n_offset + dst_pos3] = src_data[src_n_offset + src_pos3];
									}
									else if (type == Bilinear)
									{
										unsigned indexA = std::min(unsigned(src_pos3), maxIndex);
										unsigned indexB = std::min(unsigned(src_pos3 + 1), maxIndex);
										unsigned indexC = std::min(unsigned(src_pos3 + width), maxIndex);
										unsigned indexD = std::min(unsigned(src_pos3 + (width + 1)), maxIndex);
										Dtype A = src_data[src_n_offset + indexA];
										Dtype B = src_data[src_n_offset + indexB];
										Dtype C = src_data[src_n_offset + indexC];
										Dtype D = src_data[src_n_offset + indexD];

										dst_data[dst_n_offset + dst_pos3] = Dtype(static_cast<float>(A) * (1 - xdiff) * (1 - ydiff) +
											static_cast<float>(B) * xdiff * (1 - ydiff) +
											static_cast<float>(C) * ydiff * (1 - xdiff) +
											static_cast<float>(D) * xdiff * ydiff);
									}
									else
									{
										LOG(ERROR) << "Un-support interpolation type.";
									}
								}
							}
						}
					}
				}
			}
			else if (src->order() == memory::NHWC)
			{
				float width_ratio = (float)width / dst_width;
				float height_ratio = (float)height / dst_height;
				float beta = 0.5f;

				dst_temp.reset(new memory::tensor<Dtype>(std::vector<int>{num, dst_height, dst_width, channels}, src->device(), src->order()));
				Dtype* dst_data = dst_temp->mutable_cpu_data();
				const Dtype* src_data = src->cpu_data();

#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int row = 0; row < dst_height; ++row)
				{
					float yf = row * height_ratio + beta;
					int y = (int)yf;
					float ydiff = yf - y;

					int src_pos1 = y * width * channels;
					int dst_pos1 = row * dst_width * channels;

					for (int col = 0; col < dst_width; ++col)
					{
						float xf = col * width_ratio + beta;
						int x = (int)xf;
						float xdiff = xf - x;

						int src_pos2 = src_pos1 + x * channels;
						int dst_pos2 = dst_pos1 + col * channels;

						for (int n = 0; n < num; n++)
						{
							int src_n_offset = n * src_num_offset;
							int dst_n_offset = n * dst_num_offset;

							for (int ch = 0; ch < channels; ++ch)
							{
								int src_pos3 = src_pos2 + ch;
								int dst_pos3 = dst_pos2 + ch;

								if (type == Nearest)
								{
									dst_data[dst_n_offset + dst_pos3] = src_data[src_n_offset + src_pos3];
								}
								else if (type == Bilinear)
								{
									unsigned indexA = std::min(unsigned(src_pos3), maxIndex);
									unsigned indexB = std::min(unsigned(src_pos3 + channels), maxIndex);
									unsigned indexC = std::min(unsigned(src_pos3 + width * channels), maxIndex);
									unsigned indexD = std::min(unsigned(src_pos3 + (width + 1) * channels), maxIndex);
									Dtype A = src_data[src_n_offset + indexA];
									Dtype B = src_data[src_n_offset + indexB];
									Dtype C = src_data[src_n_offset + indexC];
									Dtype D = src_data[src_n_offset + indexD];

									dst_data[dst_n_offset + dst_pos3] = Dtype(static_cast<float>(A) * (1 - xdiff) * (1 - ydiff) +
										static_cast<float>(B) * xdiff * (1 - ydiff) +
										static_cast<float>(C) * ydiff * (1 - xdiff) +
										static_cast<float>(D) * xdiff * ydiff);
								}
								else
								{
									LOG(ERROR) << "Un-support interpolation type.";
								}
							}
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
#endif // !_OPERATION_MAKE_BORDER_HPP_

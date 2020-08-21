#pragma once
#ifndef _OPERATION_ROTATE_HPP_
#define _OPERATION_ROTATE_HPP_
#include <memory>
#include <Primitives/logger.hpp>

#include "operation_resize.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template <typename Dtype>
		class point
		{
		public:
			Dtype x;
			Dtype y;

			point()
			{
				x = Dtype(0);
				y = Dtype(0);
			}

			point(Dtype x, Dtype y)
			{
				this->x = x;
				this->y = y;
			}

			point& operator=(const point& r)
			{
				if (this == &r)
				{
					return *this;
				}
				x = r.x;
				y = r.y;
				return *this;
			}

			point(const point& r)
			{
				x = r.x;
				y = r.y;
			}

			float distance(const point& r)
			{
				return sqrt((x - r.x) * (x - r.x) * 1.0f + (y - r.y) * (y - r.y) * 1.0f);
			}
		};

		//calculate value of |A|
		static double getA(std::vector<std::vector<double> > arcs, int n)
		{
			if (n == 1)
			{
				return arcs[0][0];
			}
			double ans = 0;
			std::vector<std::vector<double> > temp(n, std::vector<double>(n));
			int i, j, k;
			for (i = 0; i < n; i++)
			{
				for (j = 0; j < n - 1; j++)
				{
					for (k = 0; k < n - 1; k++)
					{
						temp[j][k] = arcs[j + 1][(k >= i) ? k + 1 : k];

					}
				}
				double t = getA(temp, n - 1);
				if (i % 2 == 0)
				{
					ans += arcs[0][i] * t;
				}
				else
				{
					ans -= arcs[0][i] * t;
				}
			}
			return ans;
		}

		//calculate adjoint matrix A*
		static void getAStart(std::vector<std::vector<double> > arcs, int n, std::vector<std::vector<double> >& ans)
		{
			if (n == 1)
			{
				ans[0][0] = 1;
				return;
			}
			int i, j, k, t;
			std::vector<std::vector<double> > temp(n, std::vector<double>(n));
			for (i = 0; i < n; i++)
			{
				for (j = 0; j < n; j++)
				{
					for (k = 0; k < n - 1; k++)
					{
						for (t = 0; t < n - 1; t++)
						{
							temp[k][t] = arcs[k >= i ? k + 1 : k][t >= j ? t + 1 : t];
						}
					}


					ans[j][i] = getA(temp, n - 1);
					if ((i + j) % 2 == 1)
					{
						ans[j][i] = -ans[j][i];
					}
				}
			}
		}

		//calculate invert matrix A^(-1)
		static bool GetMatrixInverse(std::vector<std::vector<double> > src, std::vector<std::vector<double> >& des)
		{
			int n = src.size();
			des.resize(n);
			for (size_t i = 0; i < n; i++)
			{
				des[i].resize(n);
			}

			double flag = getA(src, n);
			std::vector<std::vector<double> > t(n, std::vector<double>(n));

			if (flag == 0)
			{
				return false;
			}
			else
			{
				getAStart(src, n, t);
				for (int i = 0; i < n; i++)
				{
					for (int j = 0; j < n; j++)
					{
						des[i][j] = t[i][j] / flag;
					}

				}
			}

			return true;
		}
		
		/// <summary>
		/// rotate around any point, height and width will be constant
		/// </summary>
		/// <param name="src">original memory::tensor</param>
		/// <param name="dst">new memory::tensor</param>
		/// <param name="center">rotation center</param>
		/// <param name="theta">rotation angle, anti-clockwise is positive</param>
		/// <param name="scale">ratio of scale, 1 by default</param>
		/// <param name="fill_pixel_value">pixel value to fill in the blank area, zero by default</param>
		/// <param name="type">interpolationType: Nearest / Bilinear(default)</param>
		template <typename Dtype, typename Ptype>
		static void rotate_with_points_cpu(const std::shared_ptr<memory::tensor<Dtype>> &src, std::shared_ptr<memory::tensor<Dtype>> &dst,
			const point<Ptype> &center, float theta, float scale = 1.0f, Dtype fill_pixel_value = 0, interpolationType type = Bilinear)
		{
			if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}

			if (fabs(theta) <= 1e-6)
			{
				dst = std::make_shared<memory::tensor<Dtype>>(src->clone());
				return;
			}

			int num = src->num();
			int channels = src->channels();
			int height = src->height();
			int width = src->width();
			int offset = height * width;
			int num_offset = channels * height * width;
			unsigned maxIndex = height * width * channels - 1;

			std::shared_ptr<memory::tensor<Dtype>> dst_temp;
			dst_temp.reset(new memory::tensor<Dtype>(src->data_shape(), src->device(), src->order(), src->allocator()));
			const Dtype* src_data = src->cpu_data();
			Dtype* dst_data = dst_temp->mutable_cpu_data();

			double rad = theta*(PI / 180);
			double cosa = cos(rad);
			double sina = sin(rad);

			double a = scale * cosa;
			double b = scale * sina;
			std::vector<std::vector<double> > M;
			std::vector<std::vector<double> > reverse_M;
			M.resize(3);

			for (int i = 0; i < M.size(); i++)
			{
				M[i].resize(3);
			}

			M[0][0] = a;
			M[0][1] = b;
			M[0][2] = (1 - a) * (double)center.x - b * (double)center.y;
			M[1][0] = -1 * b;
			M[1][1] = a;
			M[1][2] = b * (double)center.x + (1 - a) * (double)center.y;
			M[2][0] = 0;
			M[2][1] = 0;
			M[2][2] = 1;

			bool isInverted = GetMatrixInverse(M, reverse_M);
			if (!isInverted)
			{
				LOG(FATAL) << "cannot rotate!!!";
				return;
			}

			if (src->order() == memory::NCHW)
			{
				for (int n = 0; n < num; n++)
				{
					int n_offset = n * num_offset;

					for (int ch = 0; ch < channels; ++ch)
					{
						int channel_offset = ch * offset;
#ifdef _OPENMP
#pragma omp parallel for
#endif
						for (int row = 0; row < height; ++row)
						{
							double temp_xf = reverse_M[0][1] * row + reverse_M[0][2];
							double temp_yf = reverse_M[1][1] * row + reverse_M[1][2];
							int temp_dst_index = channel_offset + row * width;

							for (int col = 0; col < width; ++col)
							{
								double xf = reverse_M[0][0] * col + temp_xf;
								double yf = reverse_M[1][0] * col + temp_yf;
								int x = (int)xf;
								int y = (int)yf;
								float xdiff = xf - x;
								float ydiff = yf - y;

								int src_index = channel_offset + y * width + x;
								int dst_index = temp_dst_index + col;

								if (x < 0 || x >= width || y < 0 || y >= height)
								{
									dst_data[n_offset + dst_index] = fill_pixel_value;
								}
								else
								{
									if (type == Nearest)
									{
										dst_data[n_offset + dst_index] = src_data[n_offset + src_index];
									}
									else if (type == Bilinear)
									{
										unsigned indexA = std::min(unsigned(src_index), maxIndex);
										unsigned indexB = std::min(unsigned(src_index + 1), maxIndex);
										unsigned indexC = std::min(unsigned(src_index + width), maxIndex);
										unsigned indexD = std::min(unsigned(src_index + width + 1), maxIndex);
										Dtype A = src_data[n_offset + indexA];
										Dtype B = src_data[n_offset + indexB];
										Dtype C = src_data[n_offset + indexC];
										Dtype D = src_data[n_offset + indexD];

										dst_data[n_offset + dst_index] = Dtype(static_cast<float>(A) * (1 - xdiff) * (1 - ydiff) +
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
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int row = 0; row < height; ++row)
				{
					double temp_xf = reverse_M[0][1] * row + reverse_M[0][2];
					double temp_yf = reverse_M[1][1] * row + reverse_M[1][2];
					int dst_pos1 = row * width * channels;

					for (int col = 0; col < width; ++col)
					{
						double xf = reverse_M[0][0] * col + temp_xf;
						double yf = reverse_M[1][0] * col + temp_yf;
						int x = (int)xf;
						int y = (int)yf;
						float xdiff = xf - x;
						float ydiff = yf - y;

						int src_pos1 = (y * width + x) * channels;
						int dst_pos2 = dst_pos1 + col * channels;

						for (int n = 0; n < num; n++)
						{
							int n_offset = n * num_offset;
							for (int ch = 0; ch < channels; ++ch)
							{
								int src_pos2 = src_pos1 + ch;
								int dst_pos3 = dst_pos2 + ch;

								if (x < 0 || x >= width || y < 0 || y >= height)
								{
									dst_data[n_offset + dst_pos3] = fill_pixel_value;
								}
								else
								{
									if (type == Nearest)
									{
										dst_data[n_offset + dst_pos3] = src_data[n_offset + src_pos2];
									}
									else if (type == Bilinear)
									{
										unsigned indexA = std::min(unsigned(src_pos2), maxIndex);
										unsigned indexB = std::min(unsigned(src_pos2 + channels), maxIndex);
										unsigned indexC = std::min(unsigned(src_pos2 + width * channels), maxIndex);
										unsigned indexD = std::min(unsigned(src_pos2 + (width + 1) * channels), maxIndex);
										Dtype A = src_data[n_offset + indexA];
										Dtype B = src_data[n_offset + indexB];
										Dtype C = src_data[n_offset + indexC];
										Dtype D = src_data[n_offset + indexD];

										dst_data[n_offset + dst_pos3] = Dtype(static_cast<float>(A) * (1 - xdiff) * (1 - ydiff) +
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
			else
			{
				NOT_IMPLEMENTED;
			}

			dst = std::make_shared<memory::tensor<Dtype>>(dst_temp->clone());
		}
	}
}
#endif
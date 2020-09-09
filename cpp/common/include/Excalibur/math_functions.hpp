#pragma once
#ifndef _MATH_FUNCTIONS_HPP_
#define _MATH_FUNCTIONS_HPP_
#include "Primitives/basic_math.hpp"
#include <vector>

namespace glasssix
{
	namespace excalibur
	{
		class math_functions
		{
		public:
			math_functions();
			~math_functions();

			static void cpu_sgemm(const CBLAS_TRANSPOSE TransA,
				const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
				const float alpha, const float* A, const float* B, const float beta,
				float* C);

			/*static void cpu_fgemm(const CBLAS_TRANSPOSE TransA,
				const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
				const float alpha, const signed char* A, const signed char* B, const float beta,
				int* C);*/

			static void cpu_sgemv(const CBLAS_TRANSPOSE TransA, const int M,
				const int N, const float alpha, const float* A, const float* x,
				const float beta, float* y);

#ifdef USE_MKL
			static void cpu_batch_sgemm(const CBLAS_TRANSPOSE TransA, const CBLAS_TRANSPOSE TransB,
				const int M, const int N, const int K, const float alpha, const float* A, const int A_offset, const float* B, const int B_offset, const float beta, float* C, const int C_offset, int num);
#endif

			template <typename Dtype>
			static void excalibur_copy(const int N, const Dtype *X, Dtype *Y, int device)
			{
				if (X != Y)
				{
					if (device >= 0)
					{
#ifdef USE_CUDA
						CUDA_CHECK(cudaSetDevice(device));
						CUDA_CHECK(cudaMemcpy(Y, X, sizeof(Dtype) * N, cudaMemcpyDefault));
#else
						NO_GPU;
#endif
					}
					else
					{
						memcpy(Y, X, sizeof(Dtype) * N);
					}
				}
			}

			static void cpu_set(const int N, const float alpha, float* Y);

			static void cpu_sqr(const int N, const float* a, float* y);

			static void cpu_abs(const int N, const float* a, float* y);

			template <typename Dtype>
			static void cpu_add(const int N, const Dtype* a, const Dtype* b, Dtype* y);

			template <typename Dtype>
			static void cpu_sub(const int N, const Dtype* a, const Dtype* b, Dtype* y);

			template <typename Dtype>
			static void cpu_mul(const int N, const Dtype* a, const Dtype* b, Dtype* y);

			template <typename Dtype>
			static void cpu_div(const int N, const Dtype* a, const Dtype* b, Dtype* y);

			//solve equation by Gauss Elimination Method(global principal component sort), only support: (M, N) * (N, 1) = (M, 1)
			static std::vector<float> gauss_all(std::vector<std::vector<float> > A, std::vector<float> B)
			{
				int length = B.size();
				int i, j, k, maxi, maxj;
				double lik, temp;

				CHECK_EQ(length, A.size());
				for (size_t i = 0; i < A.size(); i++)
				{
					CHECK_EQ(length, A[i].size());
				}
				std::vector<float> X(length);
				std::vector<int> recordColExchange;

				for (k = 0; k<length - 1; k++)
				{
					for (maxi = maxj = i = k; i<length; i++)
					{
						for (j = k; j<length; j++)
							if (A[i][j]>A[maxi][maxj])
							{
								maxi = i;
								maxj = j;
							}
					}
					if (maxi != k)//exchange_row(A, B, k, maxi);
					{
						for (j = 0; j<length; j++)
						{
							temp = A[k][j];
							A[k][j] = A[maxi][j];
							A[maxi][j] = temp;
						}

						temp = B[k];
						B[k] = B[maxi];
						B[maxi] = temp;
					}
					if (maxj != k)//exchange_a_col(A, maxj, k); //exchange cols
					{
						for (i = 0; i<length; i++)
						{
							temp = A[i][maxj];
							A[i][maxj] = A[i][k];
							A[i][k] = temp;
						}

						recordColExchange.push_back(k);
						recordColExchange.push_back(maxj);
						recordColExchange.push_back(999999);//mark
					}
					for (i = k + 1; i<length; i++)
					{
						lik = A[i][k] / A[k][k];
						for (j = k; j<length; j++)
							A[i][j] = A[i][j] - A[k][j] * lik;
						B[i] = B[i] - B[k] * lik;
					}
				}

				if (A[length - 1][length - 1] != 0)
				{
					double sum_ax;
					X[length - 1] = B[length - 1] / A[length - 1][length - 1];
					for (i = length - 2; i >= 0; i--)
					{
						for (j = i + 1, sum_ax = 0; j<length; j++)
							sum_ax += A[i][j] * X[j];
						X[i] = (B[i] - sum_ax) / A[i][i];
					}

					for (i = recordColExchange.size() - 1; i >= 0; i--)
					{
						if (recordColExchange[i] == 999999)
						{
							temp = X[recordColExchange[i - 1]];
							X[recordColExchange[i - 1]] = X[recordColExchange[i - 2]];
							X[recordColExchange[i - 2]] = temp;
						}
					}
				}

				return X;
			}

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
				for (i = 0; i<n; i++)
				{
					for (j = 0; j<n - 1; j++)
					{
						for (k = 0; k<n - 1; k++)
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
			static void getAStart(std::vector<std::vector<double> > arcs, int n, std::vector<std::vector<double> > &ans)
			{
				if (n == 1)
				{
					ans[0][0] = 1;
					return;
				}
				int i, j, k, t;
				std::vector<std::vector<double> > temp(n, std::vector<double>(n));
				for (i = 0; i<n; i++)
				{
					for (j = 0; j<n; j++)
					{
						for (k = 0; k<n - 1; k++)
						{
							for (t = 0; t<n - 1; t++)
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
			static bool GetMatrixInverse(std::vector<std::vector<double> > src, std::vector<std::vector<double> > &des)
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
					for (int i = 0; i<n; i++)
					{
						for (int j = 0; j<n; j++)
						{
							des[i][j] = t[i][j] / flag;
						}

					}
				}

				return true;
			}

#ifdef  USE_CUDA
			static void gpu_sgemm(cublasHandle_t &cublas_handle_, const CBLAS_TRANSPOSE TransA,
				const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
				const float alpha, const float* A, const float* B, const float beta,
				float* C);

			static void gpu_gemmEx(cublasHandle_t &cublas_handle_, const CBLAS_TRANSPOSE TransA,
				const CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
				const signed char* A, const signed char* B, int* C);

			static void gpu_saxpy(cublasHandle_t &cublas_handle_, const int N, const float alpha, const float* X, float* Y);

			static void gpu_set(const int N, const float alpha, float* Y);

			static void gpu_powx(const int n, const float* a, const float b, float* y);

			static void gpu_abs(const int n, const float* a, float* y);
#endif
		};
	}
}
#endif // _MATH_FUNCTIONS_HPP_
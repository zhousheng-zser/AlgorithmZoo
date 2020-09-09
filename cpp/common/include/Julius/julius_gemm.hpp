#pragma once
#ifndef _JULIUS_GEMM_HPP_
#define _JULIUS_GEMM_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			void cblas_sgemm_AnoTrans_BnoTrans(const int M, const int N, const int K, const float alpha, const float* A, const int lda,
				const float* B, const int ldb, const float beta, float* C, const int ldc);

			void cblas_sgemm_ATrans_BnoTrans(const int M, const int N, const int K, const float alpha, const float* A, const int lda,
				const float* B, const int ldb, const float beta, float* C, const int ldc);

			void cblas_sgemm_AnoTrans_BTrans(const int M, const int N, const int K, const float alpha, const float* A, const int lda,
				const float* B, const int ldb, const float beta, float* C, const int ldc);

			void cblas_sgemm_ATrans_BTrans(const int M, const int N, const int K, const float alpha, const float* A, const int lda,
				const float* B, const int ldb, const float beta, float* C, const int ldc);

			void cblas_fgemm_AnoTrans_BnoTrans(const int M, const int N, const int K, const float alpha, const signed char* A, const int lda,
				const signed char* B, const int ldb, const float beta, int* C, const int ldc);

			void cblas_fgemm_ATrans_BnoTrans(const int M, const int N, const int K, const float alpha, const signed char* A, const int lda,
				const signed char* B, const int ldb, const float beta, int* C, const int ldc);

			void cblas_fgemm_AnoTrans_BTrans(const int M, const int N, const int K, const float alpha, const signed char* A, const int lda,
				const signed char* B, const int ldb, const float beta, int* C, const int ldc);

			void cblas_fgemm_ATrans_BTrans(const int M, const int N, const int K, const float alpha, const signed char* A, const int lda,
				const signed char* B, const int ldb, const float beta, int* C, const int ldc);
		};
	}
}
 
#endif // !_JULIUS_GEMM_HPP_

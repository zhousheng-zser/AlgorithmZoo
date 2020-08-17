#pragma once
#ifndef _JULIUS_GEMV_HPP_
#define _JULIUS_GEMV_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			void cblas_sgemv_AnoTrans(const int M, const int N, const float alpha, const float  *A, const int lda,
				const float  *x, const int incx, const float beta, float  *y, const int incy);

			void cblas_sgemv_ATrans(const int M, const int N, const float alpha, const float  *A, const int lda,
				const float  *x, const int incx, const float beta, float  *y, const int incy);
		}
	}
}
#endif // !_JULIUS_GEMV_HPP_

#pragma once
#ifndef _JULIUS_AXPBY_HPP_
#define _JULIUS_AXPBY_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			void cblas_saxpby(const int n, const float alpha, const float* x,
				const int incx, const float beta, float* y, const int incy);

			void cblas_daxpby(const int n, const double alpha, const double* x,
				const int incx, const double beta, double* y, const int incy);
		}
	}
}
#endif // !_JULIUS_AXPBY_HPP_

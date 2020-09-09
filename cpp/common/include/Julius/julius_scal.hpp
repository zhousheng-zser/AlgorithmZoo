#pragma once
#ifndef _JULIUS_SCAL_HPP_
#define _JULIUS_SCAL_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			void cblas_sscal(const int n, const float alpha, float *x, const int incx);

			void cblas_dscal(const int n, const double alpha, double *x, const int incx);
		}
	}
}
#endif // !_JULIUS_SCAL_HPP_

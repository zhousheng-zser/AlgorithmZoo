#pragma once
#ifndef _JULIUS_DOT_HPP_
#define _JULIUS_DOT_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace  juliusblas
		{
			float  cblas_sdot(const int n, const float  *x, const int incx, const float  *y, const int incy);

			double cblas_ddot(const int n, const double *x, const int incx, const double *y, const int incy);
		}
	}
}
#endif // !_JULIUS_DOT_HPP_

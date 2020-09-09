#pragma once
#ifndef _JULIUS_DSDOT_HPP_
#define _JULIUS_DSDOT_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			float  cblas_sdsdot(const int n, const float alpha, const float *x, const int incx, const float *y, const int incy);
			double cblas_dsdot(const int n, const float *x, const int incx, const float *y, const int incy);
		}
	}
}
#endif // !_JULIUS_DSDOT_HPP_

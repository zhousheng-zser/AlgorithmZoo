#pragma once
#ifndef _JULIUS_ASUM_HPP_
#define _JULIUS_ASUM_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			float cblas_sasum(const int n, const float *x, const int incx);
			double cblas_dasum(const int n, const double *x, const int incx);
		}
	}
}
	
#endif // !_JULIUS_ASUM_HPP_

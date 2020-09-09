#pragma once
#ifndef _JULIUS_NRM2_HPP_
#define _JULIUS_NRM2_HPP_

namespace glasssix
{
	namespace excalibur
	{
		namespace juliusblas
		{
			float cblas_snrm2(const int n, const float *x, const int incx);
			
			double cblas_dnrm2(const int n, const double *x, const int incx);
		}
	}
}
 
#endif // !_JULIUS_NRM2_HPP_

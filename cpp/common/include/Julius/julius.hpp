#pragma once
#ifndef _JULIUS_HPP_
#define _JULIUS_HPP_

namespace glasssix
{
	namespace excalibur
	{
		enum CBLAS_LAYOUT { CblasRowMajor = 101, CblasColMajor = 102 };
		enum CBLAS_TRANSPOSE { CblasNoTrans = 111, CblasTrans = 112, CblasConjTrans = 113 };
		enum CBLAS_UPLO { CblasUpper = 121, CblasLower = 122 };
		enum CBLAS_DIAG { CblasNonUnit = 131, CblasUnit = 132 };
		enum CBLAS_SIDE { CblasLeft = 141, CblasRight = 142 };
		enum CBLAS_STORAGE { CblasPacked = 151 };
		enum CBLAS_IDENTIFIER { CblasAMatrix = 161, CblasBMatrix = 162 };
		enum CBLAS_OFFSET { CblasRowOffset = 171, CblasColOffset = 172, CblasFixOffset = 173 };


		//-----Level 1 Functions-----


		// The ?asum routine computes the sum of the magnitudes of elements of a real vector, 
		// or the sum of magnitudes of the real and imaginary parts of elements of a complex vector:
		// return \sum_{i = 1}^n (|\Re{x_i}| + |\Im{x_i}|)
		float cblas_sasum(const int n, const float *x, const int incx);
		// return \sum_{i = 1}^n (|\Re{x_i}| + |\Im{x_i}|)
		double cblas_dasum(const int n, const double *x, const int incx);

		// The ?axpy routines perform a vector-vector operation defined as:
		// y := alpha*x + y
		void cblas_saxpy(const int n, const float alpha, const float *x, const int incx, float *y, const int incy);
		// y := alpha*x + y
		void cblas_daxpy(const int n, const double alpha, const double *x, const int incx, double *y, const int incy);


		//The ?sdot routines compute the inner product of two vectors with double precision. 
		//Both routines use double precision accumulation of the intermediate results, 
		//but the sdsdot routine outputs the final result in single precision, 
		//whereas the dsdot routine outputs the double precision result.
		//The function sdsdot also adds scalar value sb to the inner product.
		// return: (float)\sum_{i = 1}^n (double)x_i * (double)y_i + (double)alpha
		float  cblas_sdsdot(const int n, const float alpha, const float *x, const int incx, const float *y, const int incy);
		// return: \sum_{i = 1}^n (double)x_i * (double)y_i
		double cblas_dsdot(const int n, const float *x, const int incx, const float *y, const int incy);

		// The ?dot routines perform a vector-vector reduction operation defined as:
		// return \sum_{i = 1}^n (x_i * y_i)
		float  cblas_sdot(const int n, const float  *x, const int incx, const float  *y, const int incy);
		// return \sum_{i = 1}^n (x_i * y_i)
		double cblas_ddot(const int n, const double *x, const int incx, const double *y, const int incy);

		// The ?nrm2 routines perform a vector reduction operation defined as:
		// return \|x\|_2
		float cblas_snrm2(const int n, const float *x, const int incx);
		// return \|x\|_2
		double cblas_dnrm2(const int n, const double *x, const int incx);

		// The ?scal routines perform a vector operation defined as:
		// x = alpha*x
		void cblas_sscal(const int n, const float alpha, float *x, const int incx);
		// x = alpha*x
		void cblas_dscal(const int n, const double alpha, double *x, const int incx);

		//-----Level 2 Functions-----

		//The ?gemv routines perform a matrix-vector operation defined as:
		// y := alpha*op(A)*x + beta*y
		void cblas_sgemv(const enum CBLAS_LAYOUT order, const enum CBLAS_TRANSPOSE trans, const int m, const int n,
			const float alpha, const float  *a, const int lda, const float  *x, const int incx, const float beta, float  *y, const int incy);
		// y := alpha*op(A)*x + beta*y
		void cblas_dgemv(const enum CBLAS_LAYOUT order, const enum CBLAS_TRANSPOSE trans, const int m, const int n,
			const double alpha, const double  *a, const int lda, const double  *x, const int incx, const double beta, double  *y, const int incy);

		//-----Level 3 Functions-----

		//The ?gemm routines compute a scalar-matrix-matrix product and add the result to a scalar-matrix product, 
		//with general matrices. The operation is defined as
		//C := alpha*op(A)*op(B) + beta*C
		void cblas_sgemm(const enum CBLAS_LAYOUT Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB,
			const int M, const int N, const int K, const float alpha, const float* A, const int lda, const float* B, const int ldb,
			const float beta, float* C, const int ldc);
		// C := alpha*op(A)*op(B) + beta*C
		void cblas_dgemm(const enum CBLAS_LAYOUT Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB,
			const int M, const int N, const int K, const double alpha, const double* A, const int lda, const double* B, const int ldb,
			const double beta, double* C, const int ldc);


		//-----BLAS-like extensions Functions-----

		//The ?axpby routines perform a vector-vector operation defined as:
		// y := alpha*x + beta*y
		void cblas_saxpby(const int n, const float alpha, const float* x,
			const int incx, const float beta, float* y, const int incy);
		// y := alpha*x + beta*y
		void cblas_daxpby(const int n, const double alpha, const double* x,
			const int incx, const double beta, double* y, const int incy);

		//The fgemm routines compute a scalar-matrix-matrix product into int32(fixed point) type and add the result to a scalar-matrix product, 
		//with general matrices. The operation is defined as
		// C := static_cast<int>(alpha*op(static_cast<int>(A))*op(static_cast<int>(B)) + beta*C)
		void cblas_fgemm(const enum CBLAS_LAYOUT Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB,
			const int M, const int N, const int K, const float alpha, const signed char* A, const int lda, const signed char* B, const int ldb,
			const float beta, int* C, const int ldc);

		//The fgemm routines compute a scalar-matrix-matrix product into fp16(half float point) type and add the result to a scalar-matrix product, 
		//with general matrices. The operation is defined as
		// C := static_cast<signed short>(alpha*op(static_cast<float>(A))*op(static_cast<float>(B)) + beta*C)
		void cblas_hgemm(const enum CBLAS_LAYOUT Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB,
			const int M, const int N, const int K, const float alpha, const unsigned short* A, const int lda, const unsigned short* B, const int ldb,
			const float beta, unsigned short* C, const int ldc);
	}
}
#endif // !_JULIUS_HPP_




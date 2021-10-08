#pragma once
#ifndef _BLAS_HPP_
#define _BLAS_HPP_
#include "cpu.hpp"
#include "gpu.hpp"

#if defined(Parallel) && defined(USE_MKL) && defined(x86)
#include <mkl.h> // USE INTEL MKL
#elif defined(USE_OPENBLAS)
extern "C"
{
#include <cblas.h> // USE OpenBLAS
}
#else
#include "Julius/julius.hpp" //USE Glasssix Julius BLAS
#endif

// A simple way to define the vsl unary functions. The operation should
// be in the form e.g. y[i] = sqrt(a[i])
#ifndef DEFINE_VSL_UNARY_FUNC(name, operation)

#define DEFINE_VSL_UNARY_FUNC(name, operation)        \
  template <typename Dtype>                           \
  void v##name(const int n, const Dtype *a, Dtype *y) \
  {                                                   \
    CHECK_GT(n, 0);                                   \
    CHECK(a);                                         \
    CHECK(y);                                         \
    for (int i = 0; i < n; ++i)                       \
    {                                                 \
      operation;                                      \
    }                                                 \
  }                                                   \
  inline void vs##name(                               \
      const int n, const float *a, float *y)          \
  {                                                   \
    v##name<float>(n, a, y);                          \
  }

DEFINE_VSL_UNARY_FUNC(Sqr, y[i] = a[i] * a[i]);
DEFINE_VSL_UNARY_FUNC(Exp, y[i] = exp(a[i]));
DEFINE_VSL_UNARY_FUNC(Ln, y[i] = log(a[i]));
DEFINE_VSL_UNARY_FUNC(Abs, y[i] = fabs(a[i]));

#endif // !DEFINE_VSL_UNARY_FUNC(name, operation)

// A simple way to define the vsl unary functions with singular parameter b.
// The operation should be in the form e.g. y[i] = pow(a[i], b)
#ifndef DEFINE_VSL_UNARY_FUNC_WITH_PARAM(name, operation)

#define DEFINE_VSL_UNARY_FUNC_WITH_PARAM(name, operation)            \
  template <typename Dtype>                                          \
  void v##name(const int n, const Dtype *a, const Dtype b, Dtype *y) \
  {                                                                  \
    CHECK_GT(n, 0);                                                  \
    CHECK(a);                                                        \
    CHECK(y);                                                        \
    for (int i = 0; i < n; ++i)                                      \
    {                                                                \
      operation;                                                     \
    }                                                                \
  }                                                                  \
  inline void vs##name(                                              \
      const int n, const float *a, const float b, float *y)          \
  {                                                                  \
    v##name<float>(n, a, b, y);                                      \
  }

DEFINE_VSL_UNARY_FUNC_WITH_PARAM(Powx, y[i] = pow(a[i], b));

#endif // !DEFINE_VSL_UNARY_FUNC_WITH_PARAM(name, operation)

// A simple way to define the vsl binary functions. The operation should
// be in the form e.g. y[i] = a[i] + b[i]
#ifndef DEFINE_VSL_BINARY_FUNC(name, operation)

#define DEFINE_VSL_BINARY_FUNC(name, operation)                       \
  template <typename Dtype>                                           \
  void v##name(const int n, const Dtype *a, const Dtype *b, Dtype *y) \
  {                                                                   \
    CHECK_GT(n, 0);                                                   \
    CHECK(a);                                                         \
    CHECK(b);                                                         \
    CHECK(y);                                                         \
    for (int i = 0; i < n; ++i)                                       \
    {                                                                 \
      operation;                                                      \
    }                                                                 \
  }                                                                   \
  inline void vs##name(                                               \
      const int n, const float *a, const float *b, float *y)          \
  {                                                                   \
    v##name<float>(n, a, b, y);                                       \
  }

DEFINE_VSL_BINARY_FUNC(Add, y[i] = a[i] + b[i]);
DEFINE_VSL_BINARY_FUNC(Sub, y[i] = a[i] - b[i]);
DEFINE_VSL_BINARY_FUNC(Mul, y[i] = a[i] * b[i]);
DEFINE_VSL_BINARY_FUNC(Div, y[i] = a[i] / b[i]);

#endif // !DEFINE_VSL_BINARY_FUNC(name, operation)

#ifdef USE_OPENBLAS
// In addition, MKL comes with an additional function axpby that is not present
// in standard blas. We will simply use a two-step (inefficient, of course) way
// to mimic that.
inline void cblas_saxpby(const int N, const float alpha, const float *X,
                         const int incX, const float beta, float *Y,
                         const int incY)
{
  cblas_sscal(N, beta, Y, incY);
  cblas_saxpy(N, alpha, X, incX, Y, incY);
}
#endif // !USE_OPENBLAS

#if defined(USE_CUDA) && defined(x86)
#include <cublas_v2.h>
namespace glasssix
{
  static const char *cublasGetErrorString(cublasStatus_t error)
  {
    switch (error)
    {
    case CUBLAS_STATUS_SUCCESS:
      return "CUBLAS_STATUS_SUCCESS";
    case CUBLAS_STATUS_NOT_INITIALIZED:
      return "CUBLAS_STATUS_NOT_INITIALIZED";
    case CUBLAS_STATUS_ALLOC_FAILED:
      return "CUBLAS_STATUS_ALLOC_FAILED";
    case CUBLAS_STATUS_INVALID_VALUE:
      return "CUBLAS_STATUS_INVALID_VALUE";
    case CUBLAS_STATUS_ARCH_MISMATCH:
      return "CUBLAS_STATUS_ARCH_MISMATCH";
    case CUBLAS_STATUS_MAPPING_ERROR:
      return "CUBLAS_STATUS_MAPPING_ERROR";
    case CUBLAS_STATUS_EXECUTION_FAILED:
      return "CUBLAS_STATUS_EXECUTION_FAILED";
    case CUBLAS_STATUS_INTERNAL_ERROR:
      return "CUBLAS_STATUS_INTERNAL_ERROR";
#if CUDA_VERSION >= 6000
    case CUBLAS_STATUS_NOT_SUPPORTED:
      return "CUBLAS_STATUS_NOT_SUPPORTED";
#endif
#if CUDA_VERSION >= 6050
    case CUBLAS_STATUS_LICENSE_ERROR:
      return "CUBLAS_STATUS_LICENSE_ERROR";
#endif
    }
    return "Unknown cublas status";
  }
}

#define CUBLAS_CHECK(condition)                                                        \
  do                                                                                   \
  {                                                                                    \
    cublasStatus_t status = condition;                                                 \
    CHECK_EQ(status, CUBLAS_STATUS_SUCCESS) << " "                                     \
                                            << glasssix::cublasGetErrorString(status); \
  } while (0)
#endif // !USE_CUBLAS

#endif // !_BLAS_HPP_

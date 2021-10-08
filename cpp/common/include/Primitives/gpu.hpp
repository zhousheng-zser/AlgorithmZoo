#pragma once
#ifndef _GPU_HPP_
#define _GPU_HPP_
#include "cpu.hpp"
#include "logger.hpp"

#ifdef x86
#ifdef USE_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#ifdef USE_CUDNN
#include <cudnn.h>
#endif //!USE_CUDNN
#endif //!USE_CUDA
#endif //!x86

namespace glasssix
{
#ifdef x86
#ifdef USE_CUDA
	// CUDA: use 512 threads per block
	const int CUDA_NUM_THREADS = 512;

	// CUDA: number of blocks for threads.
	inline int CUDA_GET_BLOCKS(const int N)
	{
		return (N + CUDA_NUM_THREADS - 1) / CUDA_NUM_THREADS;
	}

	/// <summary>
	/// Get the number of CUDA support compute-capable devices
	/// </summary>
	/// <returns>The number of devices</returns>
	int get_cuda_device_count();

	/// <summary>
	/// Get the ASCII string identifying of the device_id
	/// </summary>
	/// <param name="device_id">The 0-indexed id of the cuda device</param>
	/// <returns>The number of compute-capable devices</returns>
	std::string get_cuda_device_name(int device_id);

	/// <summary>
	/// Get the latest version of CUDA supported by the driver.
	/// The version is returned as (1000 * major + 10 * minor).
	/// For example, CUDA 9.2 would be represented by 9020.
	/// If no driver is installed, then 0 is returned as the driver version.
	/// </summary>
	/// <param name="device_id">The 0-indexed id of the cuda device</param>
	/// <returns>Returns the CUDA driver version.</returns>
	int get_cuda_device_driver_version(int device_id);

	/// <summary>
	/// Get the version number of the current CUDA Runtime instance
	/// </summary>
	/// <param name="device_id">The 0-indexed id of the cuda device</param>
	/// <returns>Returns the CUDA Runtime version.</returns>
	int get_cuda_device_runtime_version(int device_id);

	/// <summary>
	/// Get are the major and minor revision numbers defining the device's compute capability.
	/// </summary>
	/// <param name="device_id">The 0-indexed id of the cuda device</param>
	/// <returns>Returns 10 * major + minor</returns>
	int get_cuda_device_capability(int device_id);

	/// <summary>
	/// Returns free and total respectively, the free and total amount of  memory available for allocation by the device in Mbytes.
	/// </summary>
	/// <param name="device_id">The 0-indexed id of the cuda device</param>
	/// <param name="total_size">Total memory in Mb</param>
	/// <param name="free_size">Free memory in Mb</param>
	void get_cuda_device_memory(int device_id, std::size_t &total_size, std::size_t &free_size);

	int get_cuda_device_cuda_core_num(int device_id);
#endif //!USE_CUDA
#endif //!x86
}

#ifdef x86
#ifdef USE_CUDA
// CUDA: various checks for different function calls.

#define CUDA_CHECK(condition)                                             \
	/* Code block avoids redefinition of cudaError_t error */             \
	do                                                                    \
	{                                                                     \
		cudaError_t error = condition;                                    \
		CHECK_EQ(error, cudaSuccess) << " " << cudaGetErrorString(error); \
	} while (0)

#define CUDA_DRIVER_CHECK(condition)                                       \
	/* Code block avoids redefinition of cudaError_t error */              \
	do                                                                     \
	{                                                                      \
		cudaError_enum error = condition;                                  \
		CHECK_EQ(error, CUDA_SUCCESS) << " " << cudaGetErrorString(error); \
	} while (0)

// CUDA: grid stride looping
#define CUDA_KERNEL_LOOP(i, n)                          \
	for (int i = blockIdx.x * blockDim.x + threadIdx.x; \
		 i < (n);                                       \
		 i += blockDim.x * gridDim.x)

// CUDA: check for error after kernel execution and exit loudly if there is one.
#define CUDA_POST_KERNEL_CHECK CUDA_CHECK(cudaPeekAtLastError())

#ifdef USE_CUDNN
#define CUDNN_VERSION_MIN(major, minor, patch) \
	(CUDNN_VERSION >= (major * 1000 + minor * 100 + patch))

#if !defined(CUDNN_VERSION) || !CUDNN_VERSION_MIN(6, 0, 0)
#error "Primitives and higher requires CUDNN version 6.0.0 or higher"
#endif

#define CUDNN_CHECK(condition)                                                                                                  \
	do                                                                                                                          \
	{                                                                                                                           \
		cudnnStatus_t status = condition;                                                                                       \
		CHECK_EQ(status, CUDNN_STATUS_SUCCESS) << " "                                                                           \
											   << cudnnGetErrorString(status) << ", device " /*<< glasssix::current_device()*/; \
	} while (0)

#define CUDNN_CHECK2(condition, arg1, arg2)                                                      \
	do                                                                                           \
	{                                                                                            \
		cudnnStatus_t status = condition;                                                        \
		CHECK_EQ(status, CUDNN_STATUS_SUCCESS) << "CUDNN error "                                 \
											   << (int)status << " " << (arg1) << " " << (arg2); \
	} while (0)
#endif
#endif //!USE_CUDA
#endif //!x86

#endif // !_GPU_HPP_

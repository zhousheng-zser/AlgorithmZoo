#pragma once
#ifndef _GPU_COMMON_HPP_
#define _GPU_COMMON_HPP_
#include "gpu.hpp"
#include "blas.hpp"

namespace glasssix
{
	std::uint32_t lwp_id();
	std::uint64_t lwp_dev_id(int dev = -1);

	template<typename Dtype>
	void atomic_maximum(std::atomic<Dtype>& max_val, Dtype const& new_val) noexcept {
		Dtype prev_val = std::atomic_load(&max_val);
		while (prev_val < new_val &&
			!max_val.compare_exchange_weak(prev_val, new_val)) {
		}
	}

	template<typename Dtype>
	void atomic_minimum(std::atomic<Dtype>& min_val, Dtype const& new_val) noexcept {
		Dtype prev_val = std::atomic_load(&min_val);
		while (prev_val > new_val &&
			!min_val.compare_exchange_weak(prev_val, new_val)) {
		}
	}

#ifdef USE_CUDA
	// Shared CUDA Stream for correct life cycle management
	class CUDAStream
	{
		explicit CUDAStream(bool high_priority);

	public:
		~CUDAStream();

		static std::shared_ptr<CUDAStream> create(bool high_priority = false)
		{
			std::shared_ptr<CUDAStream> pstream(new CUDAStream(high_priority));
			return pstream;
		}

		cudaStream_t get() const
		{
			return stream_;
		}

	private:
		cudaStream_t stream_;
		DISABLE_COPY_AND_ASSIGN(CUDAStream);
	};

	struct CUBLASHandle
	{
		explicit CUBLASHandle(std::shared_ptr<CUDAStream> stream);
		~CUBLASHandle();

		cublasHandle_t get() const
		{
			return handle_;
		}
	private:
		cublasHandle_t handle_;
		std::shared_ptr<CUDAStream> stream_;
		DISABLE_COPY_AND_ASSIGN(CUBLASHandle);
	};

#ifdef USE_CUDNN
	struct CUDNNHandle
	{
		explicit CUDNNHandle(std::shared_ptr<CUDAStream> stream);
		~CUDNNHandle();

		cudnnHandle_t get() const
		{
			return handle_;
		}
	private:
		cudnnHandle_t handle_;
		std::shared_ptr<CUDAStream> stream_;
		DISABLE_COPY_AND_ASSIGN(CUDNNHandle);
	};
#endif
#endif //!USE_CUDA
}
#endif _GPU_COMMON_HPP_ //!_GPU_COMMON_HPP_
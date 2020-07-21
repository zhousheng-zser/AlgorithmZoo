#pragma once
#ifndef _SYNCEDMEM_HPP_
#define _SYNCEDMEM_HPP_
#include "pool_allocator.hpp"

namespace glasssix
{
	namespace memory
	{
		template <typename Dtype>
		class syncedmem
		{
		public:
			syncedmem();
			explicit syncedmem(size_t size, int device = -1);
			~syncedmem();


			enum SyncedHead { UNINITIALIZED, HEAD_AT_CPU, HEAD_AT_GPU, SYNCED };
			SyncedHead head() { return head_; }

			void set_allocator(pool_allocator<Dtype>* allocator);

			size_t size() { return size_; }
			const Dtype* cpu_data();
			//void set_cpu_data(Dtype* data); // DEPTRCATED!
			const Dtype* gpu_data();
			//void set_gpu_data(Dtype* data); // DEPTRCATED!
			Dtype* mutable_cpu_data();
			Dtype* mutable_gpu_data();
#ifdef USE_CUDA
			void async_gpu_push(const cudaStream_t& stream);
#endif

		private:
			Dtype* cpu_ptr_;
			Dtype* gpu_ptr_;
			pool_allocator<Dtype>* allocator_;
			size_t size_;
			SyncedHead head_;
			bool own_cpu_data_;
			bool own_gpu_data_;
			int device_;

			void check_device();
			void to_cpu();
			void to_gpu();

		};
	}
}
#endif // !_SYNCEDMEM_HPP_

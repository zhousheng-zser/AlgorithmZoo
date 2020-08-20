#pragma once
#ifndef _POOL_ALLOCATOR_HPP_
#define _POOL_ALLOCATOR_HPP_

#include "gpu.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "dllexport.hpp"
#include "basic_allocator.hpp"

#include <type_traits>
#include <list>

namespace glasssix
{
	namespace memory
	{
		/// <summary>
		/// A stateless essential allocator to allocate memory on the heap.
		/// </summary>
		template<typename Object>
		class pool_allocator : public basic_allocator<Object, pool_allocator<Object>>
		{
			friend basic_allocator<Object, pool_allocator>;

		public:
			pool_allocator()
			{
				size_compare_ratio = 192;// 0.75f * 256
			}

			~pool_allocator()
			{
				clear();
				if (!payouts.empty())
				{
					LOG(ERROR) << "pool allocator destroyed too early.";
					std::list< std::pair<size_t, void*> >::iterator it = payouts.begin();
					for (; it != payouts.end(); it++)
					{
						void* ptr = it->second;
						LOG(FATAL) << ptr << "  still in use";
					}
				}
			}

			// ratio range 0 ~ 1
			// default cr = 0.75
			void set_size_compare_ratio(float scr)
			{
				if (scr < 0.f || scr > 1.f)
				{
					LOG(FATAL) << "invalid size compare ratio " << scr;
					return;
				}
				size_compare_ratio = (unsigned int)(scr * 256);
			}

			// release all budgets immediately
			void clear()
			{
				budgets_lock.lock();
				std::list< std::pair<size_t, void*> >::iterator it = budgets.begin();
				for (; it != budgets.end(); it++)
				{
					void* ptr = it->second;
					aligned_heap_free(ptr);
				}
				budgets.clear();
				budgets_lock.unlock();
			}

			void* fastMalloc(size_t size, int device_id = -1)
			{
				budgets_lock.lock();
				// find free budget
				std::list< std::pair<size_t, void*> >::iterator it = budgets.begin();
				for (; it != budgets.end(); it++)
				{
					size_t bs = it->first;
					// size_compare_ratio ~ 100%
					if (bs >= size && ((bs * size_compare_ratio) >> 8) <= size)
					{
						void* ptr = it->second;
						budgets.erase(it);
						budgets_lock.unlock();
						payouts_lock.lock();
						payouts.push_back(std::make_pair(bs, ptr));
						payouts_lock.unlock();
						return ptr;
					}
				}
				budgets_lock.unlock();
				// new
				void* ptr;
				if (device_id >= 0)
				{
#ifdef USE_CUDA
					cudaSetDevice(device_id);
					CUDA_CHECK(cudaMallocHost(&ptr, size));
#else
					NO_GPU;
#endif
				}
				else
				{
					ptr = aligned_heap_alloc(size);
				}
				payouts_lock.lock();
				payouts.push_back(std::make_pair(size, ptr));
				payouts_lock.unlock();
				return ptr;
			}

			void fastFree(void* ptr, int device_id = -1)
			{
				payouts_lock.lock();
				// return to budgets
				std::list< std::pair<size_t, void*> >::iterator it = payouts.begin();
				for (; it != payouts.end(); it++)
				{
					if (it->second == ptr)
					{
						size_t size = it->first;
						payouts.erase(it);
						payouts_lock.unlock();
						budgets_lock.lock();
						budgets.push_back(std::make_pair(size, ptr));
						budgets_lock.unlock();
						return;
					}
				}
				payouts_lock.unlock();
				LOG(ERROR) << "pool allocator get wild " << ptr;
				if (device_id >= 0)
				{
#ifdef USE_CUDA
					CUDA_CHECK(cudaFreeHost(ptr));
#else
					NO_GPU;
#endif
				}
				else
				{
					aligned_heap_free(ptr);
				}
			}

		private:
			std::mutex budgets_lock;
			std::mutex payouts_lock;
			unsigned int size_compare_ratio;// 0~256
			std::list< std::pair<size_t, void*> > budgets;
			std::list< std::pair<size_t, void*> > payouts;
		};

		template<typename UnderlyingType>
		struct EXPORT_EXCALIBUR_PRIMITIVES pool_allocator_default
		{
			static pool_allocator<UnderlyingType>& get();
		};
}
}
#endif // !_POOL_ALLOCATOR_HPP_

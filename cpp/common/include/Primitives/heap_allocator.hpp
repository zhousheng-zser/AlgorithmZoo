#pragma once

#include "memory.hpp"
#include "basic_allocator.hpp"

#include <type_traits>

namespace glasssix
{
	namespace memory
	{
		/// <summary>
		/// A stateless essential allocator to allocate memory on the heap.
		/// </summary>
		template<typename Object>
		class heap_allocator : public basic_allocator<Object, heap_allocator<Object>>
		{
			friend basic_allocator<Object, heap_allocator>;

			/// <summary>
			/// Allocates a piece of memory.
			/// </summary>
			/// <param name="size">The size in bytes</param>
			/// <returns>The pointer</returns>
			byte_type* allocate_bytes_impl(std::size_t size)
			{
				return heap_alloc_elements<byte_type>(size);
			}

			/// <summary>
			/// Deallocates a piece of memory.
			/// </summary>
			/// <param name="ptr">The pointer</param>
			/// <param name="size">The size in bytes</param>
			void deallocate_bytes_impl(byte_type* ptr, std::size_t size)
			{
				heap_free(ptr, size);
			}
		};
	}
}

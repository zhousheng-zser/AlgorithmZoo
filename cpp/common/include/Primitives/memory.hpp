#pragma once

#include "dllexport.hpp"

#include <utility>
#include <cstdint>
#include <cstddef>
#include <type_traits>

// For pointer alignment
const std::size_t MALLOC_ALIGN = 32;

namespace glasssix
{
	namespace memory
	{
#if _HAS_STD_BYTE
		using byte_type = std::byte;
#else
		using byte_type = std::uint8_t;
#endif

		/// <summary>
		/// Terminates the process immediately.
		/// </summary>
		/// <remarks>
		/// Throwing exceptions across DLL boundaries is very dangerous for possible different C++ standard libraries.
		/// Thus, we simply terminate the process if any fatal error occurs.
		/// </remarks>
		[[noreturn]] EXPORT_EXCALIBUR_PRIMITIVES void glasssix_terminate() noexcept;

		/// <summary>
		/// Allocates a piece of memory on the heap.
		/// </summary>
		/// <param name="size">The size in bytes</param>
		/// <returns>The memory pointer</returns>
		EXPORT_EXCALIBUR_PRIMITIVES void* heap_alloc(std::size_t size) noexcept;

		/// <summary>
		/// Frees a piece of memory on the heap.
		/// </summary>
		/// <param name="memory">The memory pointer</param>
		EXPORT_EXCALIBUR_PRIMITIVES void heap_free(void* memory) noexcept;

		/// <summary>
		/// Frees a piece of memory on the heap.
		/// </summary>
		/// <param name="memory">The memory pointer</param>
		/// <param name="size">The size in bytes</param>
		/// <returns>The memory pointer</returns>
		EXPORT_EXCALIBUR_PRIMITIVES void heap_free(void* memory, std::size_t size) noexcept;

		/// <summary>
		/// Allocates a piece of memory with aligned size.
		/// </summary>
		/// <param name="size">The size in bytes</param>
		/// <param name="alignment">Pointer alignment size</param>
		/// <returns>The aligned-pointer</returns>
		EXPORT_EXCALIBUR_PRIMITIVES void* aligned_heap_alloc(std::size_t size, std::size_t alignment = MALLOC_ALIGN);

		/// <summary>
			/// Deallocates a piece of aligned memory.
			/// </summary>
			/// <param name="memblock">The aligned-pointer</param>
		EXPORT_EXCALIBUR_PRIMITIVES void aligned_heap_free(void *memblock);

		/// <summary>
		/// Allocates a piece of memory which contains elements of the specified type on the heap.
		/// </summary>
		/// <typeparam name="Element">The element type</typeparam>
		/// <param name="size">The size of elements</param>
		/// <returns>The memory pointer</returns>
		template<typename Element>
		Element* heap_alloc_elements(std::size_t size)
		{
			return static_cast<Element*>(heap_alloc(size * sizeof(Element)));
		}

		/// <summary>
		/// Allocates some objects with constructible arguments.
		/// </summary>
		/// <typeparam name="Object">The object type</typeparam>
		/// <typeparam name="...Args">The argument types</typeparam>
		/// <param name="...args">The arguments</param>
		/// <returns>The memory pointer at the first object</returns>
		template<typename Object, typename... Args>
		auto heap_alloc_objects(std::size_t size, Args&&... args) -> std::enable_if_t<std::is_constructible<Object, Args...>::value, Object*>
		{
			auto result = static_cast<Object*>(heap_alloc(sizeof(Object) * size));
			auto end_ptr = result + size;

			for (auto ptr = result; ptr < end_ptr; ptr++)
			{
				new (ptr) Object{ std::forward<Args>(args)... };
			}

			return result;
		}

		/// <summary>
		/// Allocates an object with constructible arguments.
		/// </summary>
		/// <typeparam name="Object">The object type</typeparam>
		/// <typeparam name="...Args">The argument types</typeparam>
		/// <param name="...args">The arguments</param>
		/// <returns>The object pointer</returns>
		template<typename Object, typename... Args>
		auto heap_alloc_object(Args&&... args) -> std::enable_if_t<std::is_constructible<Object, Args...>::value, Object*>
		{
			auto result = static_cast<Object*>(heap_alloc(sizeof(Object)));

			return new (result) Object{ std::forward<Args>(args)... };
		}

		/// <summary>
		/// Destroys an object with freeing the memory on the heap.
		/// </summary>
		/// <typeparam name="Object">The object type</typeparam>
		/// <param name="memory">The memory</param>
		template<typename Object>
		void heap_free_object(Object* memory)
		{
			if (memory != nullptr)
			{
				memory->~Object();
				heap_free(memory);
			}
		}
	}
}

#pragma once

#include "memory.hpp"

#include <cstddef>
#include <type_traits>

namespace glasssix
{
	namespace memory
	{
		namespace details
		{
			template<typename T, typename... Args>
			struct contains_template_argument : std::false_type {};

			template<template<typename...> class Template, typename... Args>
			struct contains_template_argument<Template<Args...>, Args...> : std::true_type {};
		}

		/// <summary>
		/// A stateless essential allocator with the Curiously Recurring Template Pattern (CRTP).
		/// </summary>
		template<typename Object, typename Impl, typename = std::enable_if_t<details::contains_template_argument<Impl, Object>::value>>
		class basic_allocator
		{
		public:
			using pointer_type = std::add_pointer_t<Object>;

			/// <summary>
			/// Allocates a piece of memory which contains elements with the type of the allocator.
			/// </summary>
			/// <param name="size">The size in elements</param>
			/// <returns>The pointer at the first element</returns>
			pointer_type allocate(std::size_t size)
			{
				return reinterpret_cast<pointer_type>(allocate_bytes(size * sizeof(Object)));
			}

			/// <summary>
			/// Deallocates a piece of memory which contains elements with the type of the allocator.
			/// </summary>
			/// <param name="ptr">The pointer at the first element</param>
			/// <param name="size">The size in elements</param>
			void deallocate(pointer_type ptr, std::size_t size)
			{
				deallocate_bytes(reinterpret_cast<byte_type*>(ptr), size * sizeof(Object));
			}

			/// <summary>
			/// Constructs an object with the arguments of its constructor.
			/// </summary>
			/// <typeparam name="Individual">The object type</typeparam>
			/// <typeparam name="...Args">The types of the arguments of its constructor</typeparam>
			/// <param name="ptr">The object</param>
			/// <param name="...args">The arguments of its constructor</param>
			template<typename Individual, typename... Args>
			auto construct(Individual* ptr, Args&&... args) -> std::enable_if_t<std::is_constructible<Individual, Args...>::value>
			{
				if (ptr == nullptr)
				{
					glasssix_terminate();
				}

				::new (ptr) Individual{ std::forward<Args>(args)... };
			}

			/// <summary>
			/// Destroys an object.
			/// </summary>
			/// <typeparam name="Individual">The object type</typeparam>
			/// <param name="ptr">The object</param>
			template<typename Individual>
			void destroy(Individual* ptr)
			{
				if (ptr != nullptr)
				{
					ptr->~Individual();
				}
			}

		private:
			/// <summary>
			/// Allocates a piece of memory.
			/// </summary>
			/// <param name="size">The size in bytes</param>
			/// <returns>The pointer</returns>
			byte_type* allocate_bytes(std::size_t size)
			{
				return static_cast<Impl&>(*this).allocate_bytes_impl(size);
			}

			/// <summary>
			/// Deallocates a piece of memory.
			/// </summary>
			/// <param name="ptr">The pointer</param>
			/// <param name="size">The size in bytes</param>
			void deallocate_bytes(byte_type* ptr, std::size_t size)
			{
				static_cast<Impl&>(*this).deallocate_bytes_impl(ptr, size);
			}
		};
	}
}

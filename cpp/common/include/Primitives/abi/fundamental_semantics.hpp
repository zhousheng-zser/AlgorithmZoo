#pragma once

#include "g6_attributes.hpp"

#include <atomic>
#include <cstdint>
#include <exception>
#include <type_traits>

namespace glasssix::exposing
{
	/// <summary>
	/// Indicates that an argument accepts a void pointer containing an ABI which is prepared to be taken over by another object.
	/// </summary>
	class take_over_abi_from_void_ptr
	{
	public:
		constexpr take_over_abi_from_void_ptr(void* abi) noexcept : abi_{ abi }
		{
		}

		constexpr operator void* () const noexcept
		{
			return abi_;
		}

		constexpr take_over_abi_from_void_ptr& operator=(void* abi) noexcept
		{
			return (abi_ = abi, *this);
		}

		friend constexpr bool operator==(const take_over_abi_from_void_ptr& left, void* right) noexcept
		{
			return left.abi_ == right;
		}

		friend constexpr bool operator==(void* left, const take_over_abi_from_void_ptr& right) noexcept
		{
			return left == right.abi_;
		}

		friend constexpr bool operator==(const take_over_abi_from_void_ptr& left, const take_over_abi_from_void_ptr& right) noexcept
		{
			return left.abi_ == right.abi_;
		}

		friend constexpr bool operator!=(const take_over_abi_from_void_ptr& left, void* right) noexcept
		{
			return !(left == right);
		}

		friend constexpr bool operator!=(void* left, const take_over_abi_from_void_ptr& right) noexcept
		{
			return !(left == right);
		}

		friend constexpr bool operator!=(const take_over_abi_from_void_ptr& left, const take_over_abi_from_void_ptr& right) noexcept
		{
			return !(left == right);
		}

		template<typename Pointer, typename = std::enable_if_t<std::is_pointer_v<Pointer>>>
		constexpr Pointer to() const noexcept
		{
			return static_cast<Pointer>(abi_);
		}
	private:
		void* abi_;
	};

	/// <summary>
	/// A thread-safe reference counter using memory fence technology.
	/// </summary>
	class atomic_ref_count
	{
	public:
		atomic_ref_count() noexcept : ref_count_{ 0 }
		{
		}

		atomic_ref_count(std::uint32_t ref_count) noexcept : ref_count_{ ref_count }
		{
		}

		atomic_ref_count& operator=(std::uint32_t right) noexcept
		{
			return (ref_count_.store(right, std::memory_order_relaxed), *this);
		}

		std::uint32_t operator++() noexcept
		{
			// For self-increasing, we only care the single atomic operation so that the order is relaxed.
			return ref_count_.fetch_add(1, std::memory_order_relaxed) + 1;
		}

		std::uint32_t operator--() noexcept
		{
			std::uint32_t old_count = ref_count_.fetch_sub(1, std::memory_order_release);

			// Gets rid of underflow which is absolutely forbidden.
			if (old_count == 0)
			{
				std::terminate();
			}

			std::uint32_t count = old_count - 1;

			if (count == 0)
			{
				// Ensures the memory order of freeing.
				std::atomic_thread_fence(std::memory_order_acquire);
			}

			return count;
		}

		operator std::uint32_t() const noexcept
		{
			return ref_count_.load(std::memory_order_relaxed);
		}
	private:
		std::atomic_uint32_t ref_count_;
	};

	/// <summary>
	/// Gets the current modular reference count.
	/// </summary>
	/// <returns>The modular reference count</returns>
	inline atomic_ref_count& get_module_ref_count() noexcept
	{
		static atomic_ref_count ref_count{ 0 };

		return ref_count;
	}
}

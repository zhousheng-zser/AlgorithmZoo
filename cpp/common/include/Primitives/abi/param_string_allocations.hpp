#pragma once

#include "dllexport.hpp"
#include "g6_attributes.hpp"
#include "platform_encoding.hpp"
#include "pure_c_handle_utils.h"
#include "fundamental_semantics.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>

namespace glasssix::exposing::allocations
{
	DEFINE_PURE_C_HANDLE(param_string);

	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL create_param_string(const utf8_char * str, std::size_t size) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL create_param_string_from_narrow(const char* narrow_str, std::size_t size) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL create_param_string_ref(param_string_handle str) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL duplicate_param_string(param_string_handle str) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL concat_c_string_with_param_string(const utf8_char * left, param_string_handle right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL concat_param_string_with_c_string(param_string_handle left, const utf8_char * right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES param_string_handle G6_ABI_CALL concat_param_string(param_string_handle left, param_string_handle right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES bool G6_ABI_CALL compare_c_string_with_param_string(const utf8_char * left, param_string_handle right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES bool G6_ABI_CALL compare_param_string_with_c_string(param_string_handle left, const utf8_char * right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES bool G6_ABI_CALL compare_param_string(param_string_handle left, param_string_handle right) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::uint32_t G6_ABI_CALL free_param_string(param_string_handle str) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES const utf8_char * G6_ABI_CALL get_param_string_data(param_string_handle str) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::size_t G6_ABI_CALL get_param_string_size(param_string_handle str) noexcept;
}

namespace glasssix::exposing
{
	/// <summary>
	/// An basic string that holds a reference to a real param_string.
	/// </summary>
	class basic_param_string
	{
	public:
		basic_param_string(std::nullptr_t) noexcept : handle_{}
		{
		}
			 
		basic_param_string(take_over_abi_from_void_ptr abi) noexcept : handle_{ abi.to<allocations::param_string_handle>() }
		{
		}

		basic_param_string(utf8_string_view str) noexcept : handle_{ allocations::create_param_string(str.data(), str.size()) }
		{
		}

		basic_param_string(const basic_param_string& other) noexcept : handle_{ allocations::create_param_string_ref(other.handle_) }
		{
		}

		basic_param_string(basic_param_string&& other) noexcept : handle_{ std::exchange(other.handle_, nullptr) }
		{
		}

		~basic_param_string() noexcept
		{
			clear();
		}

		basic_param_string& operator=(const basic_param_string& right) noexcept
		{
			return (clear(), handle_ = allocations::create_param_string_ref(right.handle_), *this);
		}

		basic_param_string& operator=(basic_param_string&& right) noexcept
		{
			return (clear(), handle_ = std::exchange(right.handle_, nullptr), *this);
		}

		explicit operator bool() const noexcept
		{
			return handle_;
		}

		operator utf8_string_view() const noexcept
		{
			return handle_ ? utf8_string_view{ allocations::get_param_string_data(handle_), allocations::get_param_string_size(handle_) } : utf8_string_view{};
		}

		void clear() noexcept
		{
			if (handle_)
			{
				allocations::free_param_string(handle_);
				handle_ = nullptr;
			}
		}

		allocations::param_string_handle get_add_ref() const noexcept
		{
			return allocations::create_param_string_ref(handle_);
		}
	protected:
		allocations::param_string_handle handle_;
	};

	/// <summary>
	/// Converts a string to a platform-dependent narrow string.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The narrow string</returns>
	inline std::string to_narrow_string(const utf8_string_view str) noexcept
	{
		return platform_encoding::utf8_to_narrow(str);
	}

	/// <summary>
	/// Overloads the << operator of a std::ostream.
	/// </summary>
	/// <param name="left">The left</param>
	/// <param name="right">The right</param>
	/// <returns>The self reference</returns>
	inline std::ostream& operator<<(std::ostream& left, const basic_param_string& right)
	{
		return (left << to_narrow_string(right));
	}

	/// <summary>
	/// Gets the ABI of a basic string with type information erased.
	/// </summary>
	/// <param name="str">The basic string</param>
	/// <returns>The ABI</returns>
	inline void* get_abi(const basic_param_string& str) noexcept
	{
		return meta::get_standard_layout_first_member<allocations::param_string_handle>(str);
	}

	/// <summary>
	/// Gets a pointer to the ABI of a basic string with type information erased.
	/// The ABI will not be cleared and the caller must ensure safety.
	/// </summary>
	/// <param name="str">The basic string</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi_dangerous(basic_param_string& str) noexcept
	{
		return reinterpret_cast<void**>(&meta::get_standard_layout_first_member<allocations::param_string_handle>(str));
	}

	/// <summary>
	/// Gets a pointer to the ABI of a basic string with type information erased.
	/// </summary>
	/// <param name="str">The basic string</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi(basic_param_string& str) noexcept
	{
		return (str.clear(), put_abi_dangerous(str));
	}

	/// <summary>
	/// Detaches the ABI from a basic string.
	/// </summary>
	/// <param name="str">The basic string</param>
	/// <returns>The ABI detached from the opaque string</returns>
	inline void* detach_abi(basic_param_string& str) noexcept
	{
		return std::exchange(*put_abi_dangerous(str), nullptr);
	}

	/// <summary>
	/// Detaches the ABI from a basic string.
	/// </summary>
	/// <param name="str">The basic string</param>
	/// <returns>The ABI detached from the basic string</returns>
	inline void* detach_abi(basic_param_string&& str) noexcept
	{
		return std::exchange(*put_abi_dangerous(str), nullptr);
	}

	/// <summary>
	/// Creates a basic string from an ABI with the reference count increased.
	/// </summary>
	/// <typeparam name="T">The string type</typeparam>
	/// <param name="abi">The ABI</param>
	/// <returns>The basic string</returns>
	template<typename T, std::enable_if_t<std::is_same_v<T, basic_param_string>>* = nullptr>
	T create_from_abi(void* abi) noexcept
	{
		return T{ take_over_abi_from_void_ptr{ allocations::create_param_string_ref(static_cast<allocations::param_string_handle>(abi)) } };
	}
}

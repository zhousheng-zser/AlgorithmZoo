#pragma once

#include "guid.hpp"
#include "meta.hpp"
#include "hash_utils.hpp"
#include "exceptions.hpp"
#include "platform_encoding.hpp"
#include "fundamental_semantics.hpp"
#include "param_string_allocations.hpp"

#include "fmt/format.h"

#include <string>
#include <cstddef>
#include <ostream>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <string_view>

namespace glasssix::exposing
{
	struct abi_result;

	class param_string;

	template<typename String, typename... Args>
	param_string format(String&& format_str, Args&&... args);

	template<typename Callable>
	abi_result abi_safe_call(Callable&& handler) noexcept;
	void check_abi_result(abi_result result);
}

namespace glasssix::exposing
{
	/// <summary>
	/// An ABI-safe string for parameters.
	/// </summary>
	class param_string
	{
	public:
		using value_type = utf8_char;
		using view_type = utf8_string_view;
		using const_iterator = const value_type*;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		/// <summary>
		/// Creates an instance.
		/// </summary>
		param_string() noexcept : param_string{ u8"" }
		{
		}

		/// <summary>
		/// Creates an instance with nullptr.
		/// This overload is only for initialization purpose only.
		/// </summary>
		param_string(std::nullptr_t) : handle_{ nullptr }
		{
		}

		/// <summary>
		/// Create an instance with an ABI from which ownership is taken.
		/// </summary>
		/// <param name="abi">The ABI</param>
		param_string(take_over_abi_from_void_ptr abi) noexcept : handle_{ abi.to<allocations::param_string_handle>() }
		{
		}

		/// <summary>
		/// Creates an instance.
		/// </summary>
		/// <param name="str">The string</param>
		param_string(const value_type* str) noexcept : param_string{ view_type{ str } }
		{
		}

		/// <summary>
		/// Creates an instance.
		/// </summary>
		/// <param name="str">The string</param>
		param_string(view_type str) noexcept : param_string{ str.data(), str.size() }
		{
		}

		/// <summary>
		/// Creates an instance from a basic string.
		/// </summary>
		/// <param name="str">The basic string</param>
		param_string(const basic_param_string& str) : param_string{ take_over_abi_from_void_ptr{ str.get_add_ref() } }
		{
		}

		/// <summary>
		/// Creates an instance.
		/// </summary>
		/// <param name="str">The string</param>
		/// <param name="size">The size</param>
		param_string(const value_type* str, std::size_t size) noexcept : handle_{ allocations::create_param_string(str, size) }
		{
		}

		param_string(const param_string& other) noexcept : handle_{ allocations::create_param_string_ref(other.handle_) }
		{
		}

		param_string(param_string&& other) noexcept : handle_{ std::exchange(other.handle_, nullptr) }
		{
		}

		~param_string() noexcept
		{
			clear();
		}

		param_string& operator=(const param_string& right) noexcept
		{
			return (clear(), handle_ = allocations::create_param_string_ref(right.handle_), *this);
		}

		param_string& operator=(param_string&& right) noexcept
		{
			return (clear(), handle_ = std::exchange(right.handle_, nullptr), *this);
		}

		friend bool operator==(const utf8_char* left, const param_string& right) noexcept
		{
			return allocations::compare_c_string_with_param_string(left, right.handle_);
		}

		friend bool operator==(const param_string& left, const utf8_char* right) noexcept
		{
			return allocations::compare_param_string_with_c_string(left.handle_, right);
		}

		friend bool operator==(utf8_string_view left, const param_string& right) noexcept
		{
			return allocations::compare_c_string_with_param_string(left.data(), right.handle_);
		}

		friend bool operator==(const param_string& left, utf8_string_view right) noexcept
		{
			return allocations::compare_param_string_with_c_string(left.handle_, right.data());
		}

		friend bool operator==(const param_string& left, const param_string& right) noexcept
		{
			return allocations::compare_param_string(left.handle_, right.handle_);
		}

		friend bool operator!=(const utf8_char* left, const param_string& right) noexcept
		{
			return !(left == right);
		}

		friend bool operator!=(const param_string& left, const utf8_char* right) noexcept
		{
			return !(left == right);
		}

		friend bool operator!=(utf8_string_view left, const param_string& right) noexcept
		{
			return !(left == right);
		}

		friend bool operator!=(const param_string& left, utf8_string_view right) noexcept
		{
			return !(left == right);
		}

		friend bool operator!=(const param_string& left, const param_string& right) noexcept
		{
			return !(left == right);
		}

		friend param_string operator+(const utf8_char* left, const param_string& right) noexcept
		{
			return param_string{ take_over_abi_from_void_ptr{ allocations::concat_c_string_with_param_string(left, right.handle_) } };
		}

		friend param_string operator+(const param_string& left, const utf8_char* right) noexcept
		{
			return param_string{ take_over_abi_from_void_ptr{ allocations::concat_param_string_with_c_string(left.handle_, right) } };
		}

		friend param_string operator+(utf8_string_view left, const param_string& right) noexcept
		{
			return param_string{ take_over_abi_from_void_ptr{ allocations::concat_c_string_with_param_string(left.data(), right.handle_) } };
		}

		friend param_string operator+(const param_string& left, utf8_string_view right) noexcept
		{
			return param_string{ take_over_abi_from_void_ptr{ allocations::concat_param_string_with_c_string(left.handle_, right.data()) } };
		}

		friend param_string operator+(const param_string& left, const param_string& right) noexcept
		{
			return param_string{ take_over_abi_from_void_ptr{ allocations::concat_param_string(left.handle_, right.handle_) } };
		}

		/// <summary>
		/// Indicates whether the string buffer is null.
		/// </summary>
		/// <returns>True if the string buffer is null; otherwise false</returns>
		explicit operator bool() const noexcept
		{
			return handle_;
		}

		/// <summary>
		/// Provides access to certain element.
		/// </summary>
		/// <param name="index">The index</param>
		/// <returns>A const reference to the element</returns>
		const value_type& operator[](std::size_t index) const
		{
			if (index < size())
			{
				return data()[index];
			}

			throw abi_out_of_bounds{ format(FMT_STRING(u8"Index: {}, Size: {}"), index, size()) };
		}

		/// <summary>
		/// Supports casting to std::basic_string_view.
		/// </summary>
		/// <returns>The result</returns>
		operator view_type() const noexcept
		{
			return view_type{ data(), size() };
		}

		/// <summary>
		/// Checks whether the string is empty.
		/// </summary>
		/// <returns>True if the string is empty; otherwise false</returns>
		bool empty() const noexcept
		{
			return size() == 0;
		}

		/// <summary>
		/// Clears the string.
		/// </summary>
		void clear() noexcept
		{
			if (handle_)
			{
				allocations::free_param_string(handle_);
				handle_ = nullptr;
			}
		}

		/// <summary>
		/// Gets the data of the string.
		/// </summary>
		/// <returns>The data</returns>
		const value_type* data() const noexcept
		{
			return allocations::get_param_string_data(handle_);
		}

		/// <summary>
		/// Gets the size of the string.
		/// </summary>
		/// <returns>The size</returns>
		std::size_t size() const noexcept
		{
			return allocations::get_param_string_size(handle_);
		}

		/// <summary>
		/// Gets an iterator at the beginning.
		/// </summary>
		/// <returns>The iterator</returns>
		const_iterator begin() const noexcept
		{
			return data();
		}

		/// <summary>
		/// Gets a reverse iterator at the beginning.
		/// </summary>
		/// <returns>The iterator</returns>
		const_reverse_iterator rbegin() const noexcept
		{
			return const_reverse_iterator{ end() };
		}

		/// <summary>
		/// Gets a const iterator at the beginning.
		/// </summary>
		/// <returns>The iterator</returns>
		const_iterator cbegin() const noexcept
		{
			return begin();
		}

		/// <summary>
		/// Gets a const-reverse iterator at the beginning.
		/// </summary>
		/// <returns>The iterator</returns>
		const_reverse_iterator crbegin() const noexcept
		{
			return rbegin();
		}

		/// <summary>
		/// Gets an iterator at the end.
		/// </summary>
		/// <returns>The iterator</returns>
		const_iterator end() const noexcept
		{
			return data() + size();
		}

		/// <summary>
		/// Gets a reverse iterator at the end.
		/// </summary>
		/// <returns>The iterator</returns>
		const_reverse_iterator rend() const noexcept
		{
			return const_reverse_iterator{ begin() };
		}

		/// <summary>
		/// Gets an iterator at the end.
		/// </summary>
		/// <returns>The iterator</returns>
		const_iterator cend() const noexcept
		{
			return end();
		}

		/// <summary>
		/// Gets a const-reverse iterator at the end.
		/// </summary>
		/// <returns>The iterator</returns>
		const_reverse_iterator crend() const noexcept
		{
			return rend();
		}
	private:
		allocations::param_string_handle handle_;
	};

	/// <summary>
	/// Converts a narrow string to a string.
	/// </summary>
	/// <param name="narrow_str">The narrow string</param>
	/// <returns>The string</returns>
	inline param_string to_param_string(std::string_view narrow_str) noexcept
	{
		return param_string{ take_over_abi_from_void_ptr{  allocations::create_param_string_from_narrow(narrow_str.data(), narrow_str.size()) } };
	}

	/// <summary>
	/// Converts a number to a string.
	/// </summary>
	/// <typeparam name="Number">The numeric type</typeparam>
	/// <param name="number">The number</param>
	/// <returns>The string</returns>
	template<typename Number, typename = std::enable_if_t<std::is_arithmetic_v<Number>>>
	param_string to_param_string(Number number)
	{
		return to_param_string(std::to_string(number));
	}

	/// <summary>
	/// Converts a GUID to a string.
	/// </summary>
	/// <param name="id">The GUID</param>
	/// <param name="hyphenated">A boolean that indicates whether the string is hyphenated</param>
	/// <returns>The string</returns>
	inline param_string to_param_string(const guid& id, bool hyphenated = true) noexcept
	{
		auto chars = to_char_array(id, hyphenated);

		return to_param_string(std::string_view{ chars.data(), chars.size() });
	}

	/// <summary>
	/// Formats one or more arguments to a string.
	/// </summary>
	/// <typeparam name="String">The string type</typeparam>
	/// <typeparam name="...Args">The argument types</typeparam>
	/// <param name="format_str">The format string</param>
	/// <param name="...args">The arguments</param>
	/// <returns>The result</returns>
	template<typename String, typename... Args>
	param_string format(String&& format_str, Args&&... args)
	{
		param_string result{ nullptr };

		return (check_abi_result(abi_safe_call([&] { result = param_string{ fmt::format(std::forward<String>(format_str), std::forward<Args>(args)...) }; })), result);
	}

	/// <summary>
	/// Overloads the << operator of a std::ostream.
	/// </summary>
	/// <param name="left">The left</param>
	/// <param name="right">The right</param>
	/// <returns>The self reference</returns>
	inline std::ostream& operator<<(std::ostream& left, const param_string& right)
	{
		return (left << to_narrow_string(right));
	}

	/// <summary>
	/// Gets the ABI of a string with type information erased.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The ABI</returns>
	inline void* get_abi(const param_string& str) noexcept
	{
		return meta::get_standard_layout_first_member<allocations::param_string_handle>(str);
	}

	/// <summary>
	/// Gets a pointer to the ABI of a string with type information erased.
	/// The ABI will not be cleared and the caller must ensure safety.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi_dangerous(param_string& str) noexcept
	{
		return reinterpret_cast<void**>(&meta::get_standard_layout_first_member<allocations::param_string_handle>(str));
	}

	/// <summary>
	/// Gets a pointer to the ABI of a string with type information erased.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi(param_string& str) noexcept
	{
		return (str.clear(), put_abi_dangerous(str));
	}

	/// <summary>
	/// Detaches the ABI from a string.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The ABI detached from the string</returns>
	inline void* detach_abi(param_string& str) noexcept
	{
		return std::exchange(*put_abi_dangerous(str), nullptr);
	}

	/// <summary>
	/// Detaches the ABI from a string.
	/// </summary>
	/// <param name="str">The string</param>
	/// <returns>The ABI detached from the string</returns>
	inline void* detach_abi(param_string&& str) noexcept
	{
		return std::exchange(*put_abi_dangerous(str), nullptr);
	}

	/// <summary>
	/// Creates a string from an ABI with the reference count increased.
	/// </summary>
	/// <typeparam name="T">The string type</typeparam>
	/// <param name="abi">The ABI</param>
	/// <returns>The string</returns>
	template<typename T, std::enable_if_t<std::is_same_v<T, param_string>>* = nullptr>
	T create_from_abi(void* abi) noexcept
	{
		return T{ take_over_abi_from_void_ptr{ allocations::create_param_string_ref(static_cast<allocations::param_string_handle>(abi)) } };
	}
}

namespace glasssix::exposing::impl
{
	template<typename T, typename = void>
	struct has_to_param_string : std::false_type {};

	template<typename T>
	struct has_to_param_string<T, std::enable_if_t<std::conjunction_v<std::is_same<decltype(to_param_string(std::declval<T>())), param_string>, std::negation<std::is_same<T, param_string>>>>> : std::true_type {};

	/// <summary>
	/// Checks whether there is a overloaded to_param_string function for a type.
	/// </summary>
	template<typename T>
	inline constexpr bool has_to_param_string_v = has_to_param_string<T>::value;
}

namespace std
{
	template<> struct hash<glasssix::exposing::param_string>
	{
		std::size_t operator()(const glasssix::exposing::param_string& str) const
		{
			std::size_t result = 0;

			return (std::for_each(str.begin(), str.end(), [&](glasssix::exposing::utf8_char item) { glasssix::utils::hash_combine(result, item); }), result);
		}
	};
}

#pragma once

#include "meta.hpp"
#include "dllexport.hpp"
#include "g6_attributes.hpp"

#include <string>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <string_view>

#ifndef _WIN32
#include <algorithm>
#endif

namespace glasssix::exposing
{
#if __cpp_char8_t >= 201811L
	using utf8_char = char8_t;
#else
	using utf8_char = char;
#endif

	using utf8_string = std::basic_string<utf8_char>;
	using utf8_string_view = std::basic_string_view<utf8_char>;
}

namespace glasssix::exposing::platform_encoding::api::win32
{
#ifdef _WIN32
	/// <summary>
	/// Commonly used code pages defined in Windows.
	/// </summary>
	enum class encoding_codepage : std::uint32_t
	{
		system_default = 0,
		gbk = 936,
		gb18030 = 54936,
		utf8 = 65001
	};

	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::size_t G6_ABI_CALL get_narrow_to_wide_size(encoding_codepage codepage, const char* narrow_str, std::size_t size) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::size_t G6_ABI_CALL get_wide_to_narrow_size(encoding_codepage codepage, const wchar_t* wide_str, std::size_t size) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::size_t G6_ABI_CALL narrow_to_wide(encoding_codepage codepage, const char* narrow_str, std::size_t narrow_size, wchar_t* wide_char, std::size_t wide_size) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::size_t G6_ABI_CALL wide_to_narrow(encoding_codepage codepage, const wchar_t* wide_str, std::size_t wide_size, char* narrow_char, std::size_t narrow_size) noexcept;
#endif
}

namespace glasssix::exposing::platform_encoding::win32
{
#ifdef _WIN32
	template<typename T, typename BufferGetter, typename FailureCleaner, typename = std::enable_if_t<std::is_default_constructible_v<meta::tuple_first_t<std::invoke_result_t<BufferGetter, std::size_t>>>>>
	auto multibyte_to_wide(std::basic_string_view<T> multibyte_str, bool utf8, BufferGetter&& getter, FailureCleaner&& cleaner) noexcept
	{
		using result_type = meta::tuple_first_t<std::invoke_result_t<BufferGetter, std::size_t>>;

		auto code_page = utf8 ? api::win32::encoding_codepage::utf8 : api::win32::encoding_codepage::system_default;
		std::size_t wide_size = api::win32::get_narrow_to_wide_size(code_page, multibyte_str.data(), multibyte_str.size());

		if (wide_size <= 0)
		{
			return result_type{};
		}

		auto [buffer, wide_string_buffer_getter] = std::forward<BufferGetter>(getter)(wide_size);

		if (api::win32::narrow_to_wide(code_page, multibyte_str.data(), multibyte_str.size(), wide_string_buffer_getter(buffer), wide_size) <= 0)
		{
			std::forward<FailureCleaner>(cleaner)(buffer);
		}

		return buffer;
	}

	template<typename BufferGetter, typename FailureCleaner, typename = std::enable_if_t<std::is_default_constructible_v<meta::tuple_first_t<std::invoke_result_t<BufferGetter, std::size_t>>>>>
	auto wide_to_multibyte(std::wstring_view wide_str, bool utf8, BufferGetter&& getter, FailureCleaner&& cleaner) noexcept
	{
		using result_type = meta::tuple_first_t<std::invoke_result_t<BufferGetter, std::size_t>>;

		auto code_page = utf8 ? api::win32::encoding_codepage::utf8 : api::win32::encoding_codepage::system_default;
		std::size_t multibyte_size = api::win32::get_wide_to_narrow_size(code_page, wide_str.data(), wide_str.size());

		if (multibyte_size <= 0)
		{
			return result_type{};
		}

		auto [buffer, multibyte_string_buffer_getter] = std::forward<BufferGetter>(getter)(multibyte_size);

		if (api::win32::wide_to_narrow(code_page, wide_str.data(), wide_str.size(), reinterpret_cast<char*>(multibyte_string_buffer_getter(buffer)), multibyte_size) <= 0)
		{
			std::forward<FailureCleaner>(cleaner)(buffer);
		}

		return buffer;
	}

	/// <summary>
	/// Converts a platform-independent UTF-8 string to a platform-dependent wide string.
	/// </summary>
	/// <param name="utf8_str">The UTF-8 string</param>
	/// <returns>The wide string</returns>
	inline std::wstring utf8_to_wide(utf8_string_view utf8_str) noexcept
	{
		return multibyte_to_wide(
			utf8_str,
			true,
			[](std::size_t size) { return std::tuple{ std::wstring(size, L'\0'), [](std::wstring& wide_str) { return wide_str.data(); } }; },
			[](std::wstring& wide_str) { wide_str.clear(); }
		);
	}

	/// <summary>
	/// Converts a platform-dependent narrow string to a platform-dependent wide string.
	/// </summary>
	/// <param name="narrow_str">The narrow string</param>
	/// <returns>The wide string</returns>
	inline std::wstring narrow_to_wide(std::string_view narrow_str) noexcept
	{
		return multibyte_to_wide(
			narrow_str,
			false,
			[](std::size_t size) { return std::tuple{ std::wstring(size, L'\0'), [](std::wstring& wide_str) { return wide_str.data(); } }; },
			[](std::wstring& wide_str) { wide_str.clear(); }
		);
	}

	/// <summary>
	/// Converts a platform-dependent wide string to a platform-independent UTF-8 string.
	/// </summary>
	/// <param name="wide_str">The wide string</param>
	/// <returns>The UTF-8 string</returns>
	inline utf8_string wide_to_utf8(std::wstring_view wide_str) noexcept
	{
		return wide_to_multibyte(
			wide_str,
			true,
			[](std::size_t size) { return std::tuple{ utf8_string(size, '\0'), [](utf8_string& utf8_str) { return utf8_str.data(); } }; },
			[](utf8_string& utf8_str) { utf8_str.clear(); }
		);
	}

	/// <summary>
	/// Converts a platform-dependent wide string to a platform-dependent narrow string.
	/// </summary>
	/// <param name="narrow_str">The wide string</param>
	/// <returns>The narrow string</returns>
	inline std::string wide_to_narrow(std::wstring_view wide_str) noexcept
	{
		return wide_to_multibyte(
			wide_str,
			false,
			[](std::size_t size) { return std::tuple{ std::string(size, '\0'), [](std::string& narrow_str) { return narrow_str.data(); } }; },
			[](std::string& narrow_str) { narrow_str.clear(); }
		);
	}
#endif
}

namespace glasssix::exposing::platform_encoding
{
	/// <summary>
	/// Converts a UTF-8 string to a narrow string.
	/// </summary>
	/// <param name="utf8_str">The UTF-8 string</param>
	/// <returns>The narrow string</returns>
	inline std::string utf8_to_narrow(utf8_string_view utf8_str) noexcept
	{
		// On Linux the default encoding of narrow strings are UTF-8.
		// On Windows the default encoding of narrow strings are configurated as codepage.
#ifdef _WIN32
		return win32::wide_to_narrow(win32::utf8_to_wide(utf8_str));
#else
		std::string result(utf8_str.size(), '\0');

		return (std::copy(utf8_str.begin(), utf8_str.end(), result.begin()), result);
#endif
	}

	/// <summary>
	/// Converts a narrow string to a UTF-8 string.
	/// </summary>
	/// <param name="narrow_str">The narrow string</param>
	/// <returns>The UTF-8 string</returns>
	inline utf8_string narrow_to_utf8(std::string_view narrow_str) noexcept
	{
		// On Linux the default encoding of narrow strings are UTF-8.
		// On Windows the default encoding of narrow strings are configurated as codepage.
#ifdef _WIN32
		return win32::wide_to_utf8(win32::narrow_to_wide(narrow_str));
#else
		utf8_string result(narrow_str.size(), '\0');

		return (std::copy(narrow_str.begin(), narrow_str.end(), result.begin()), result);
#endif
	}
}

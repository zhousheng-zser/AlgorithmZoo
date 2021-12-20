#pragma once

#include <array>
#include <string>
#include <cstddef>
#include <type_traits>

#if __cpp_lib_char8_t >= 201907L
#define G6_SUPPORTS_LIB_CHAR8_T
#endif

namespace glasssix
{
#ifdef G6_SUPPORTS_LIB_CHAR8_T
	namespace detail
	{
		template<std::size_t N>
		struct char8_t_string_literal
		{
			static constexpr std::size_t size = N;

			bool char_literal;
			std::array<char8_t, N> sequence;

			consteval char8_t_string_literal(char8_t ch) noexcept : char_literal{ true }, sequence{ ch }
			{
			}

			template<std::size_t... Is>
			consteval char8_t_string_literal(const char8_t(&sequence)[N], std::index_sequence<Is...>) noexcept : char_literal{}, sequence{ sequence[Is]... }
			{
			}

			consteval char8_t_string_literal(const char8_t(&sequence)[N]) noexcept : char8_t_string_literal{ sequence, std::make_index_sequence<N>{} }
			{
			}
		};

		char8_t_string_literal(char8_t)->char8_t_string_literal<1>;

		template<std::size_t N>
		char8_t_string_literal(const char8_t(&)[N])->char8_t_string_literal<N>;

		template<char8_t_string_literal Literal, std::size_t... Is>
		inline constexpr const char as_char_array_v[sizeof...(Is)]{ static_cast<char>(Literal.sequence[Is])... };

		template<char8_t_string_literal Literal, std::size_t... Is>
		constexpr auto& make_as_char_array(std::index_sequence<Is...>) noexcept
		{
			return as_char_array_v<Literal, Is...>;
		}
	}

	template<detail::char8_t_string_literal Literal>
	inline constexpr decltype(auto) as_char_v = []()->decltype(auto)
	{
		if constexpr (Literal.char_literal)
		{
			return static_cast<char>(*Literal.sequence.front());
		}
		else
		{
			return detail::make_as_char_array<Literal>(std::make_index_sequence<decltype(Literal)::size>{});
		}
	}();

#define U8(x) (glasssix::as_char_v<(u8##x)>)

	inline std::string from_u8string(const std::u8string& str)
	{
		return std::string{ str.begin(), str.end() };
	}
#else
#define U8(x) u8##x
#endif

	inline std::string from_u8string(const std::string& str)
	{
		return str;
	}

	inline std::string from_u8string(std::string&& str) noexcept
	{
		return std::move(str);
	}
}

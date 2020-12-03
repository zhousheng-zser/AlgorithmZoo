#pragma once

#include <cstdint>
#include <utility>
#include <type_traits>
#include <string_view>

namespace glasssix
{
	/// <summary>
	/// Indicates the current source location.
	/// </summary>
	struct source_location
	{
		std::int32_t line;
		std::string_view file;
		std::string_view function;

		/// <summary>
		/// Accquires the current source location.
		/// </summary>
		/// <param name="line">The line</param>
		/// <param name="file">The file</param>
		/// <param name="function">The function</param>
		/// <returns>The current source location</returns>
		static constexpr source_location current(std::int32_t line = __builtin_LINE(), std::string_view file = __builtin_FILE(), std::string_view function = __builtin_FUNCTION())
		{
			return source_location{ line, file, function };
		}
	};

	template<typename T>
	struct arg_with_current_location
	{
		T arg;
		source_location location;

		template<typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
		arg_with_current_location(U&& arg, const source_location& location = source_location::current()) : arg{ std::forward<U>(arg) }, location{ location }
		{
		}
	};

	template<typename T>
	arg_with_current_location(T&&)->arg_with_current_location<T&&>;
}

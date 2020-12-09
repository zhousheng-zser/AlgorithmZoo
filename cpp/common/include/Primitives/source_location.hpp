#pragma once

#include <cstdint>
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
}

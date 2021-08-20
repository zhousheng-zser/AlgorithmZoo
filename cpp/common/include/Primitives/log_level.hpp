#pragma once

#include <cstdint>

namespace glasssix
{
	/// <summary>
	/// Available log levels.
	/// </summary>
	enum class log_level : std::int32_t
	{
		/// <summary>
		/// None will be output.
		/// </summary>
		none,

		/// <summary>
		/// A message that helps debug the program and find bugs exactly.
		/// </summary>
		debug,

		/// <summary>
		/// A message that informs the consumer of some suggestive tips.
		/// </summary>
		info,

		/// <summary>
		/// A warning that is presented to the consumer.
		/// </summary>
		warning,

		/// <summary>
		/// A serious logic error occurs now and must be resolved immediately.
		/// </summary>
		error,

		/// <summary>
		/// A fatal error occurs unexpectedly and the program must be terminated.
		/// </summary>
		fatal
	};
}

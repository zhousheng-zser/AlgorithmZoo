#pragma once

namespace glasssix::memory
{
	/// <summary>
	/// Available layouts that make sense in actual projects.
	/// </summary>
	enum class tensor_layout
	{
		rgb,
		rgba,
		grayscale,
		grayscale_3
	};
}

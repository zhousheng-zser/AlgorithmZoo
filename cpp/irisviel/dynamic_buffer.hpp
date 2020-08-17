#pragma once

#include <cstddef>
#include <cstdint>

namespace glasssix
{
	namespace irisviel
	{
		struct dynamic_buffer
		{
			virtual ~dynamic_buffer() = default;
			virtual std::size_t size() const noexcept = 0;
			virtual std::uint8_t* data() noexcept = 0;
			virtual const std::uint8_t* data() const noexcept = 0;
		};
	}
}

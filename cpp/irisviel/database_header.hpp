#pragma once

#include <atomic>
#include <memory>
#include <cstddef>
#include <functional>

namespace glasssix
{
	namespace irisviel
	{
		struct database_header
		{
			int capacity;
			int current_position;
			int version;
			char index_file_name[128] = {};
		};

		struct database_header_traits
		{
			static constexpr std::size_t current_position_offset = offsetof(database_header, current_position);
		};
	}
}

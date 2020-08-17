#pragma once

#include "Primitives/filesystem.hpp"

#include <string_view>

namespace glasssix
{
	namespace utils
	{
		bool safe_exists(const fs::path& path) noexcept;
		bool safe_directory_exists(const fs::path& path) noexcept;
		bool safe_remove_file(const fs::path& path) noexcept;
		bool safe_create_directories(const fs::path& path) noexcept;
		void safe_remove_directories(const fs::path& path) noexcept;
		fs::path path_from_string_view(std::string_view str) noexcept;
	}
}

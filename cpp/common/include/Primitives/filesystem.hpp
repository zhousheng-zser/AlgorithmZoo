#pragma once

#include <cctype>
#include <algorithm>

#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;

namespace glasssix
{
	inline bool case_insensitive_path_comare(const fs::path& left, const fs::path& right)
	{
		auto& native_left = left.native();
		auto& native_right = right.native();

		return native_left.size() == native_right.size() ? std::equal(native_left.begin(), native_left.end(), native_right.begin(), [](const fs::path::value_type left, const fs::path::value_type right) { return std::tolower(left) == std::tolower(right); }) : false;
	}
}

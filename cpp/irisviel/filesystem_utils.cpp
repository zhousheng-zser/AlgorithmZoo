#include "filesystem_utils.hpp"

namespace glasssix
{
	namespace utils
	{
		namespace
		{
			thread_local std::error_code last_error_code;
		}

		bool safe_exists(const fs::path& path) noexcept
		{
			return fs::exists(path, last_error_code);
		}

		bool safe_directory_exists(const fs::path& path) noexcept
		{
			return fs::exists(path, last_error_code) && fs::is_directory(path, last_error_code);
		}

		bool safe_remove_file(const fs::path& path) noexcept
		{
			return fs::exists(path, last_error_code) ? fs::remove(path, last_error_code) : false;
		}

		bool safe_create_directories(const fs::path& path) noexcept
		{
			return safe_directory_exists(path) ? true : fs::create_directories(path, last_error_code);
		}

		void safe_remove_directories(const fs::path& path) noexcept
		{
			fs::remove_all(path, last_error_code);
		}

		fs::path path_from_string_view(std::string_view str) noexcept
		{
			if constexpr (std::is_constructible_v<fs::path, std::string_view>)
			{
				return str;
			}
			else
			{
				// Makes it compatible with ghc::filesystem::path which does not contain a constructor with a std::string_view as a argument.
				return fs::path{ std::string{ str } };
			}
		}
	}
}

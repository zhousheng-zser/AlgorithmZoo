#pragma once

#include "log_level.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <cstdint>

namespace glasssix::logging
{
	/// <summary>
	/// The log config of the current application domain.
	/// </summary>
	struct log_config
	{
		log_level level;
		std::uint64_t max_size;
		bool enable_file_output;
		bool enable_stderr_output;
		std::string home_directory;
		std::string application_name;
		
		static log_config default_value();
		static log_config load_from_file_or_default(std::string_view path);
	};

	void to_json(nlohmann::json& json, const log_config& value);
	void from_json(const nlohmann::json& json, log_config& value);
}

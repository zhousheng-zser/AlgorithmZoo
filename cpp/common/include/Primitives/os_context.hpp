#pragma once

#include "dllexport.hpp"

#include <regex>
#include <memory>
#include <string>
#include <cstdlib>
#include <string_view>

namespace glasssix::os_context
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void free_environment_variable(char* buffer) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES char* get_environment_variable(const char* name) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES bool set_environment_variable(const char* name, const char* value) noexcept;

	inline std::string expand_enviroment_variables(std::string_view path)
	{
		thread_local std::regex pattern{ R"(\$\<(.+?)\>)", std::regex_constants::ECMAScript };
		std::string result;

		for (std::cregex_iterator iter{ path.data(), path.data() + path.size(), pattern }, iter_end; iter != iter_end; iter++)
		{
			auto origin = (*iter)[0].str();
			auto variable_name = (*iter)[1].str();

			std::shared_ptr<char> variable{ get_environment_variable(variable_name.c_str()), &free_environment_variable };

			result.append(iter->prefix());
			result.append(variable ? variable.get() : origin.c_str());
			result.append(iter->suffix());
		}

		return result.empty() ? std::string(path) : result;
	}
}

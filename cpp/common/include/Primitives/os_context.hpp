#pragma once

#include "dllexport.hpp"

#include <regex>
#include <string>
#include <cstdlib>
#include <string_view>

namespace glasssix::os_context
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES bool set_environment_variable(const char* name, const char* value) noexcept;

	inline std::string expand_enviroment_variables(std::string_view path)
	{
		thread_local std::regex pattern{ R"(\$\<(.+?)\>)", std::regex_constants::ECMAScript };
		std::string result;

		for (std::cregex_iterator iter{ path.data(), path.data() + path.size(), pattern }, iter_end; iter != iter_end; iter++)
		{
			result.append(iter->prefix());
			result.append(std::getenv((*iter)[1].str().c_str()));
			result.append(iter->suffix());
		}

		return result.empty() ? std::string(path) : result;
	}
}

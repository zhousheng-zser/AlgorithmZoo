#pragma once

#include <vector>
#include <string>

namespace glasssix::hardcode
{
	std::vector<std::string> get_model_params(std::string_view name, bool use_int8);
}

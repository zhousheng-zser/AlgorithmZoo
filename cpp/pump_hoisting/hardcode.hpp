#pragma once

#include <vector>
#include <string>

namespace glasssix::pump_hoisting
{
	std::vector<std::string> get_model_params(std::string_view name, bool use_int8 = false);
}

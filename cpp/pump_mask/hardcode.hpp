#pragma once

#include <vector>
#include <string>

namespace glasssix::pump_mask
{
	std::vector<std::string> get_model_params(std::string_view name, bool use_int8 = false);
}

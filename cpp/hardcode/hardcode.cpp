#include "hardcode.hpp"
#include <unordered_map>

namespace glasssix::hardcode
{
    namespace
    {
        struct hardcode_model_params
        {
			      
        };

        const std::unordered_map<std::string, std::vector<std::string>> hardcode_map{
        };
    }

    std::vector<std::string> get_model_params(std::string_view name, bool use_int8)
    {
        auto iter = use_int8 ? hardcode_map.find(std::string(name) + "_int8") : hardcode_map.find(std::string(name));

        return iter != hardcode_map.end() ? iter->second : std::vector<std::string>();
    }
}

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

namespace glasssix::pump_pumptop_person
{
    struct box_info_internal
    {
        std::int32_t x1;
        std::int32_t x2;
        std::int32_t y1;
        std::int32_t y2;
        std::int32_t category;
        exposing::param_vector<std::int32_t> pump;
    };

}

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

namespace glasssix::pump_weld
{
    struct box_info_internal
    {
        std::int32_t weld_x1;
        std::int32_t weld_x2;
        std::int32_t weld_y1;
        std::int32_t weld_y2;

        std::int32_t can_x1;
        std::int32_t can_x2;
        std::int32_t can_y1;
        std::int32_t can_y2;

        float score;
        std::int32_t category;
    };

}

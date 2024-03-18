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
        exposing::param_vector<std::int32_t> weldlocal_list;

        std::int32_t can_x1;
        std::int32_t can_x2;
        std::int32_t can_y1;
        std::int32_t can_y2;

        float score;
        std::int32_t category;

        box_info_internal() {
            weldlocal_list = exposing::make_param_vector<std::int32_t>();
        }
    };

}

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

namespace glasssix::facelandmarks
{
    struct land_info_internal
    {
        exposing::param_vector<exposing::param_pair<float,float>> pts;
        float score;

        land_info_internal() {
			pts = exposing::make_param_vector<exposing::param_pair<float, float>>();
        }
    };

}

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
#include "box_info.hpp"

namespace glasssix::vehicle
{
    class classify_code_internal
    {
    public:
        class impl;

        classify_code_internal(const classify_code_internal &) = delete;

        classify_code_internal &operator=(const classify_code_internal &) = delete;

        classify_code_internal(std::string_view model_directory, int device);

        virtual ~classify_code_internal();

        exposing::param_vector<vehicle::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "land_info.hpp"

namespace glasssix::facelandmarks
{
    class land_code_internal
    {
    public:
        class impl;

        land_code_internal(const land_code_internal &) = delete;

        land_code_internal &operator=(const land_code_internal &) = delete;

        land_code_internal(std::string_view model_directory, int device);

        virtual ~land_code_internal();

        facelandmarks::land_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}

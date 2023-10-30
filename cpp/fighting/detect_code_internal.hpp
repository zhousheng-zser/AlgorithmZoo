#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

namespace glasssix::fighting
{
    class detect_code_internal
    {
    public:
        class impl;

        detect_code_internal(const detect_code_internal &) = delete;

        detect_code_internal &operator=(const detect_code_internal &) = delete;

        detect_code_internal(std::string_view model_directory, int device, int batch);

        virtual ~detect_code_internal();

        float detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}

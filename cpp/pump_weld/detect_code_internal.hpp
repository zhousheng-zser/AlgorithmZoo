#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::pump_weld
{
    class detect_code_internal
    {
    public:
        class impl;

        detect_code_internal(const detect_code_internal &) = delete;

        detect_code_internal &operator=(const detect_code_internal &) = delete;

        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        exposing::param_vector<pump_weld::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int batch, int height, int width, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}

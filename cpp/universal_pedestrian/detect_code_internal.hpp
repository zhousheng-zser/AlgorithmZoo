#ifndef __UNV_PED_DETECT_CODE_INTERNAL_HPP__
#define __UNV_PED_DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::universal_pedestrian
{
    struct box_info_internal
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int category;
        float score;
        exposing::param_string version;
    };

    class detect_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        detect_code_internal(const detect_code_internal&) = delete;
        detect_code_internal& operator=(const detect_code_internal&) = delete;

        std::string version();

        exposing::param_vector<universal_pedestrian::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif
//完成
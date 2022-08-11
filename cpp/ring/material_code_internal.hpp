#ifndef __MATERIAL_CODE_INTERNAL_HPP__
#define __MATERIAL_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::ring
{

    struct box_info_internal
    {
        exposing::param_vector<float> location;
        exposing::param_vector<exposing::param_string> strinfos;
        exposing::param_vector<std::uint8_t> cut_roi;
        std::int32_t cut_roi_width;
        std::int32_t cut_roi_height;
        float angle;
    };

    class material_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        material_code_internal(std::string_view model_directory, int factory_type, int device);

        virtual ~material_code_internal();

        material_code_internal(const material_code_internal&) = delete;
        material_code_internal& operator=(const material_code_internal&) = delete;

        static std::string version();

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int border_orient, int order, 
                                                                     int x, int y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif
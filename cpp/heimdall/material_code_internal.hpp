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

namespace glasssix::heimdall
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

    /// <summary>
    /// A common component supporting anti-spoofing.
    /// </summary>
    class material_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        material_code_internal(std::string_view model_directory, int factory_type, int device, std::map<std::string, float>& param_map);

        /// <summary>
        /// The copy constructor must be disabled in PImpl pattern.
        /// </summary>
        material_code_internal(const material_code_internal &) = delete;

        /// <summary>
        /// Destroys the instance.
        /// </summary>
        virtual ~material_code_internal();

        /// <summary>
        /// The copy assignment operator must be disabled in PImpl pattern.
        /// </summary>
        material_code_internal &operator=(const material_code_internal &) = delete;

        /// <summary>
        /// Extracts the feature data.
        /// </summary>
        /// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
        /// <param name="count">The count of bitmaps in the buffer</param>
        /// <param name="order">The order that the bitmaps are arranged in</param>
        /// <returns>The feature vectors</returns>
        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height) const;

        /// <summary>
        /// Gets the version of the component.
        /// </summary>
        /// <returns>The version</returns>
        static std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif
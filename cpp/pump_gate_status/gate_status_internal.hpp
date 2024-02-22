#pragma once

#ifndef _pump_gate_status_FEATURE_HPP_
#define _pump_gate_status_FEATURE_HPP_

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#include <map>
#include <abi/param_span.hpp>
#include <abi/param_vector.hpp>

namespace glasssix::pump_gate_status
{
    struct ROI
    {
        int x1;
        int y1;
        int x2;
        int y2;
        ROI(int x1_,int y1_,int x2_,int y2_ ):x1(x1_),y1(y1_),x2(x2_),y2(y2_)
        {}

    };

    class gate_status_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>

        gate_status_internal(exposing::param_vector<int> hsvs);

        gate_status_internal();

        gate_status_internal(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8);

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="phai_path">The phai</param>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        gate_status_internal(const std::vector<std::string> &phai, std::string_view racy_path, int device);

        /// <summary>
        /// The copy constructor must be disabled in PImpl pattern.
        /// </summary>
        gate_status_internal(const gate_status_internal &) = delete;

        /// <summary>
        /// Destroys the instance.
        /// </summary>
        virtual ~gate_status_internal();

        /// <summary>
        /// The copy assignment operator must be disabled in PImpl pattern.
        /// </summary>
        gate_status_internal &operator=(const gate_status_internal &) = delete;

        /// <summary>
        /// Extracts the feature data.
        /// </summary>
        /// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
        /// <param name="count">The count of bitmaps in the buffer</param>
        /// <param name="order">The order that the bitmaps are arranged in</param>
        /// <returns>The feature vectors</returns>
        std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order = 0) const;

        int  detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int yellow_hsv_lower, int yellow_hsv_upper, int gray_hsv_lower, int gray_hsv_upper, 
            std::vector<int>& rois,
            std::map<std::string, float>&  param_map_abi) const;

        /// <summary>
        /// Gets the version of the component.
        /// </summary>
        /// <returns>The version</returns>
        static std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}

#endif // !_GAIUS_FEATURE_HPP_
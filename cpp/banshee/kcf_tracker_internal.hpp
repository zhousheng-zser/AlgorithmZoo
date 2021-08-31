#ifndef KCF_TRACKER_INTERNAL_H
#define KCF_TRACKER_INTERNAL_H

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "track_info.hpp"

namespace glasssix::banshee
{
    struct track_info_internal
    {
        int x;
        int y;
        int width;
        int height;
        float prob;
    };

    /// <summary>
    /// A common component supporting anti-spoofing.
    /// </summary>
    class kcf_tracker_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        kcf_tracker_internal(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height);

        /// <summary>
        /// The copy constructor must be disabled in PImpl pattern.
        /// </summary>
        kcf_tracker_internal(const kcf_tracker_internal &) = delete;

        /// <summary>
        /// Destroys the instance.
        /// </summary>
        virtual ~kcf_tracker_internal();

        /// <summary>
        /// The copy assignment operator must be disabled in PImpl pattern.
        /// </summary>
        kcf_tracker_internal &operator=(const kcf_tracker_internal &) = delete;

        /// <summary>
        /// Extracts the feature data.
        /// </summary>
        /// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
        /// <param name="count">The count of bitmaps in the buffer</param>
        /// <param name="order">The order that the bitmaps are arranged in</param>
        /// <returns>The feature vectors</returns>
        track_info update(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height) const;

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
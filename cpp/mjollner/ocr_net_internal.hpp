#ifndef __OCR_NET_INTERNAL_HPP__
#define __OCR_NET_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
// #include <abi/consumer.hpp>

#include "box_info.hpp"

namespace glasssix::mjollner
{
    struct box_info_internal
    {
        exposing::param_vector<float> location;
        exposing::param_string strinfo;
    };

    /// <summary>
    /// A common component supporting anti-spoofing.
    /// </summary>
    class ocr_net_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        ocr_net_internal(std::string_view det_racy_path, std::string_view rec_racy_path, std::string_view alphabet_path, int device);

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="phai_path">The phai</param>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        ocr_net_internal(const std::vector<std::string> &det_phai, std::string_view det_racy_path, const std::vector<std::string> &rec_phai, std::string_view rec_racy_path, std::string_view alphabet_path, int device);

        /// <summary>
        /// The copy constructor must be disabled in PImpl pattern.
        /// </summary>
        ocr_net_internal(const ocr_net_internal &) = delete;

        /// <summary>
        /// Destroys the instance.
        /// </summary>
        virtual ~ocr_net_internal();

        /// <summary>
        /// The copy assignment operator must be disabled in PImpl pattern.
        /// </summary>
        ocr_net_internal &operator=(const ocr_net_internal &) = delete;

        /// <summary>
        /// Extracts the feature data.
        /// </summary>
        /// <param name="bitmaps">Some bitmaps (128x128x3) arranged in specified order</param>
        /// <param name="count">The count of bitmaps in the buffer</param>
        /// <param name="order">The order that the bitmaps are arranged in</param>
        /// <returns>The feature vectors</returns>
        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

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
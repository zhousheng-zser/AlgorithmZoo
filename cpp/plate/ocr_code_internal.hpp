#ifndef __OCR_CODE_INTERNAL_HPP__
#define __OCR_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::plate
{
    struct anchor_box
    {
        float x;
        float y;
        float h;
        float w;
    };

    struct box_info_internal
    {
        anchor_box rect;
        float confidence = 1.0f;
        exposing::param_string strinfos;
        exposing::param_vector<std::uint8_t> aligned_images;
    };

    class ocr_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        ocr_code_internal(std::string_view model_directory, int device);

        virtual ~ocr_code_internal();

        ocr_code_internal(const ocr_code_internal&) = delete;
        ocr_code_internal& operator=(const ocr_code_internal&) = delete;

        static std::string version();

        exposing::param_vector<plate::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order,
            int x, int y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

        exposing::param_vector<plate::box_info> recognize(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

        void trace_init(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order, int roi_x, int roi_y, int roi_width, int roi_height);

        exposing::param_vector<plate::box_info> trace_update(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order);

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif
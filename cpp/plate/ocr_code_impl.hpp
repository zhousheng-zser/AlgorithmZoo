#ifndef __OCR_CODE_IMPL_HPP__
#define __OCR_CODE_IMPL_HPP__

#include "ocr_code.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::plate
{
    inline constexpr exposing::utf8_string_view plate_ocr_code_qualified_name{ u8"g6.plate.ocr_code" };

    class ocr_code_internal;

    class ocr_code_impl : public exposing::implements<ocr_code_impl, ocr_code>, public exposing::make_external_qualified_name<plate_ocr_code_qualified_name>
    {
    public:
        ocr_code_impl();
        ~ocr_code_impl();

        void init(const exposing::param_string& model_directory, std::int32_t device);

        exposing::param_string version() const;

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order,
                                                                     int x, int y, int roi_width, int roi_height, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const;

    private:

        std::unique_ptr<ocr_code_internal> impl_;
    };
}

#endif
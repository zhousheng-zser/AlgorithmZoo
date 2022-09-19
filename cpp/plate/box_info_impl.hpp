#ifndef __BOX_INFO_IMPL_HPP__
#define __BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "ocr_code_internal.hpp"

namespace glasssix::plate
{
    inline constexpr exposing::utf8_string_view plate_box_info_qualified_name{ u8"g6.plate.boxInfo" };

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<plate_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal& internal);
        ~box_info_impl();

        float x() const;
        float y() const;
        float width() const;
        float height() const;
        exposing::param_string strinfos() const;
        exposing::param_vector<std::uint8_t> aligned_images() const;
        float confidence() const;

        void set_x(float input);
        void set_y(float input);
        void set_width(float input);
        void set_height(float input);
        void set_confidence(float input);

    private:
        box_info_internal internal_;
    };
}
#endif
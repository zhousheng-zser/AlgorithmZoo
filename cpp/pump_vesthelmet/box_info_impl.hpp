#pragma once

#include "box_info.hpp"
#include "box_info_internal.hpp"

namespace glasssix::pump_vesthelmet
{
    inline constexpr exposing::utf8_string_view pump_vesthelmet_box_info_qualified_name{ u8"g6.pump_vesthelmet.box_info" };


    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<pump_vesthelmet_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal &internal);
        ~box_info_impl();
        std::int32_t x1();
        std::int32_t x2();
        std::int32_t y1();
        std::int32_t y2();
        float score();
        std::int32_t category();

    private:
        box_info_internal internal_;
    };
}

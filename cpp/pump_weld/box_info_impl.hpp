#pragma once

#include "box_info.hpp"
#include "box_info_internal.hpp"

namespace glasssix::pump_weld
{
    inline constexpr exposing::utf8_string_view pump_weld_box_info_qualified_name{ u8"g6.pump_weld.box_info" };


    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<pump_weld_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal &internal);
        ~box_info_impl();
        std::int32_t weld_x1();
        std::int32_t weld_x2();
        std::int32_t weld_y1();
        std::int32_t weld_y2();
        std::int32_t can_x1();
        std::int32_t can_x2();
        std::int32_t can_y1();
        std::int32_t can_y2();
        float score();
        std::int32_t category();

    private:
        box_info_internal internal_;
    };
}

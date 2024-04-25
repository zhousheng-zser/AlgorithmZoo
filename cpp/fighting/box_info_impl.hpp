#pragma once

#include "box_info.hpp"
#include "detect_code_internal.hpp"

namespace glasssix::fighting
{
    inline constexpr exposing::utf8_string_view fighting_box_info_qualified_name{ u8"g6.fighting.box_info" };


    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<fighting_box_info_qualified_name>
    {
    public:
        //box_info_impl();
        box_info_impl(const BoxInfoInternal&internal);
        ~box_info_impl();
        std::int32_t x1();
        std::int32_t x2();
        std::int32_t y1();
        std::int32_t y2();
        std::int32_t category();
        float score();
    private:
        BoxInfoInternal internal_;
    };
}

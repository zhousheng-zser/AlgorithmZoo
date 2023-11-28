#pragma once
#ifndef __BOX_INFO_IMPL_HPP__
#define __BOX_INFO_IMPL_HPP__
#include "box_info.hpp"
#include "detect_code_internal.hpp"

namespace glasssix::onphone
{
    inline constexpr exposing::utf8_string_view onphone_box_info_qualified_name{ u8"g6.onphone.box_info" };


    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<onphone_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal &internal);
        ~box_info_impl();
        std::int32_t x1();
        std::int32_t x2();
        std::int32_t y1();
        std::int32_t y2();
        float category();
        float confidence();
        exposing::param_vector<std::int32_t> phonelocal_list();
        exposing::param_vector<float> phonescore_list();

    private:
        box_info_internal internal_;
    };
}
#endif
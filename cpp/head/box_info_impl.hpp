#ifndef __head_BOX_INFO_IMPL_HPP__
#define __head_BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "detect_code_internal.hpp"

namespace glasssix::head
{
    inline constexpr exposing::utf8_string_view head_box_info_qualified_name{ u8"g6.head.boxInfo" };

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<head_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal& internal);
        ~box_info_impl();

        int x1() const;
        int y1() const;
        int x2() const;
        int y2() const;
        float score() const;
        int category() const;
        exposing::param_vector<float> key_points() const;

    private:
        box_info_internal internal_{};
    };
}
#endif
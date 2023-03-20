#ifndef __BOX_INFO_IMPL_HPP__
#define __BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "txt_code_internal.hpp"

namespace glasssix::genocr
{
    inline constexpr exposing::utf8_string_view genocr_box_info_qualified_name{u8"g6.genocr.boxInfo"};

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<genocr_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal &internal);
        ~box_info_impl();

        exposing::param_vector<float> location() const;
		exposing::param_vector<exposing::param_string> strinfos() const;
        float angle() const;

    private:
        box_info_internal internal_;
    };
}
#endif
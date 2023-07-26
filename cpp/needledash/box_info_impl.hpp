#ifndef __NEEDLEDASH_BOX_INFO_IMPL_HPP__
#define __NEEDLEDASH_BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "ocr_code_internal.hpp"

namespace glasssix::needledash
{
    inline constexpr exposing::utf8_string_view needledash_box_info_qualified_name{ u8"g6.needledash.boxInfo" };

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<needledash_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal& internal);
        ~box_info_impl();

        exposing::param_string strinfo() const;

    private:
        box_info_internal internal_;
    };
}
#endif
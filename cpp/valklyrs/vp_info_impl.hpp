#ifndef __VP_INFO_IMPL_HPP__
#define __VP_INFO_IMPL_HPP__

#include "vp_info.hpp"
#include "yolov5s_net_internal.hpp"

namespace glasssix::valklyrs
{
    inline constexpr exposing::utf8_string_view valklyrs_vp_info_qualified_name{u8"g6.valklyrs.vpInfo"};

    class vp_info_impl : public exposing::implements<vp_info_impl, vp_info>, public exposing::make_external_qualified_name<valklyrs_vp_info_qualified_name>
    {
    public:
        vp_info_impl();
        vp_info_impl(const vp_info_internal &internal);
        ~vp_info_impl();

        exposing::param_vector<float> coordinates() const;
        exposing::param_vector<exposing::param_string> attributes() const;

    private:
        vp_info_internal internal_;
    };
}
#endif
#ifndef __RESULT_INFO_IMPL_HPP__
#define __RESULT_INFO_IMPL_HPP__

#include "result_info.hpp"
#include "yolov5s_net_internal.hpp"

namespace glasssix::valklyrs
{
    inline constexpr exposing::utf8_string_view valklyrs_result_info_qualified_name{u8"g6.valklyrs.resultInfo"};

    class result_info_impl : public exposing::implements<result_info_impl, result_info>, public exposing::make_external_qualified_name<valklyrs_result_info_qualified_name>
    {
    public:
        result_info_impl();
        result_info_impl(const result_info_internal &internal);
        ~result_info_impl();

        exposing::param_vector<vp_info> vehicle_list() const;
		exposing::param_vector<vp_info> person_list() const;

    private:
        result_info_internal internal_;
    };
}
#endif
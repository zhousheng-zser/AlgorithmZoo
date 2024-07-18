#ifndef __BOX_INFO_IMPL_HPP__
#define __BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "detect_code_internal.hpp"

namespace glasssix::subway_anomaly
{
    inline constexpr exposing::utf8_string_view subway_anomaly_box_info_qualified_name{ u8"g6.subway_anomaly.boxInfo" };

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<subway_anomaly_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal& internal);
        ~box_info_impl();

        float score() const;
        bool anomaly_status() const;
        exposing::param_string version() const;

    private:
        box_info_internal internal_{};
    };
}
#endif
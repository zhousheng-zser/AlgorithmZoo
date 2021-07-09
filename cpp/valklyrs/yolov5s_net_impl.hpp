#ifndef __YOLOV5S_NET_IMPL_HPP__
#define __YOLOV5S_NET_IMPL_HPP__

#include "yolov5s_net.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::valklyrs
{
    inline constexpr exposing::utf8_string_view valklyrs_yolov5s_net_qualified_name{u8"g6.valklyrs.yolov5s_net"};

    class yolov5s_net_internal;

    class yolov5s_net_impl : public exposing::implements<yolov5s_net_impl, yolov5s_net>, public exposing::make_external_qualified_name<valklyrs_yolov5s_net_qualified_name>
    {
    public:
        yolov5s_net_impl();
        ~yolov5s_net_impl();
        void init(const exposing::param_string &yolov5s_racy_path, const exposing::param_string &vehicle_racy_path, const exposing::param_string &person_racy_path, std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> yolov5s_phai, const exposing::param_string &yolov5s_racy_path, exposing::param_span<const exposing::param_string> vehicle_phai, const exposing::param_string &vehicle_racy_path, exposing::param_span<const exposing::param_string> person_phai, const exposing::param_string &person_racy_path, std::int32_t device);
        exposing::param_string version() const;
        exposing::param_vector<result_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

    private:
        std::unique_ptr<yolov5s_net_internal> impl_;
    };
}

#endif
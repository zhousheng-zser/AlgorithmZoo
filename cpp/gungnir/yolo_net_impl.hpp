#ifndef __YOLO_NET_IMPL_HPP__
#define __YOLO_NET_IMPL_HPP__

#include "yolo_net.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::gungnir
{
    inline constexpr exposing::utf8_string_view gungnir_yolo_net_qualified_name{u8"g6.gungnir.yolo_net"};

    class yolo_net_internal;

    class yolo_net_impl : public exposing::implements<yolo_net_impl, yolo_net>, public exposing::make_external_qualified_name<gungnir_yolo_net_qualified_name>
    {
    public:
        yolo_net_impl();
        ~yolo_net_impl();
        void init(const exposing::param_string &racy_path, std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string &racy_path, std::int32_t device);
        exposing::param_string version() const;
        exposing::param_vector<hat_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

    private:
        std::unique_ptr<yolo_net_internal> impl_;
    };
}

#endif
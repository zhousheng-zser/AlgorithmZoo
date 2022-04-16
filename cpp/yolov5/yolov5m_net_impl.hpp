#ifndef __yolov5m_NET_IMPL_HPP__
#define __yolov5m_NET_IMPL_HPP__

#include "yolov5m_net.hpp"

#include <memory>
#include <utility>
#include <iostream>

#include <abi/consumer.hpp>

namespace glasssix::yolov5
{
    inline constexpr exposing::utf8_string_view yolov5_yolov5m_net_qualified_name{ u8"g6.yolov5.yolov5m_net" };

    class yolov5m_net_internal;

    class yolov5m_net_impl : public exposing::implements<yolov5m_net_impl, yolov5m_net>, public exposing::make_external_qualified_name<yolov5_yolov5m_net_qualified_name>
    {
    public:
        yolov5m_net_impl();
        ~yolov5m_net_impl();

        void init(exposing::param_string yolov5m_racy_path, std::int32_t device);
        void init(exposing::param_string yolov5m_phai, exposing::param_string yolov5m_racy_path, std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> yolov5m_phai, exposing::param_string yolov5m_racy_path, std::int32_t device);

        exposing::param_string version() const;

        yolov5::result_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order);

    private:
        
        std::unique_ptr<yolov5m_net_internal> impl_;
    };
}

#endif
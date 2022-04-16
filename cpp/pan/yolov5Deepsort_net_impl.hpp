#ifndef __yolov5Deepsort_net_IMPL_HPP__
#define __yolov5Deepsort_net_IMPL_HPP__

#include "yolov5Deepsort_net.hpp"

#include <memory>
#include <utility>
#include <iostream>

#include <abi/consumer.hpp>

namespace glasssix::pan
{
    inline constexpr exposing::utf8_string_view yolov5_yolov5Deepsort_net_qualified_name{ u8"g6.pan.yolov5Deepsort_net" };

    class yolov5Deepsort_net_internal;

    class yolov5Deepsort_net_impl : public exposing::implements<yolov5Deepsort_net_impl, yolov5Deepsort_net>, public exposing::make_external_qualified_name<yolov5_yolov5Deepsort_net_qualified_name>
    {
    public:
        yolov5Deepsort_net_impl();
        ~yolov5Deepsort_net_impl();

        void init(exposing::param_string yolov5m_racy_path, exposing::param_string deepsort_racy_path, std::int32_t device);
        void init(exposing::param_string yolov5m_phai, exposing::param_string yolov5m_racy_path, exposing::param_string deepsort_phai, exposing::param_string deepsort_racy_path,  std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> yolov5m_phai, exposing::param_string yolov5m_racy_path, exposing::param_span<const exposing::param_string> deepsort_phai, exposing::param_string deepsort_racy_path,  std::int32_t device);

        exposing::param_string version() const;

        pan::result_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order);

    private:
        
        std::unique_ptr<yolov5Deepsort_net_internal> impl_;
    };
}

#endif
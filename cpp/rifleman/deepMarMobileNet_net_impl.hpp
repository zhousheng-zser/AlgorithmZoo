#ifndef __deepMarMobileNet_net_IMPL_HPP__
#define __deepMarMobileNet_net_IMPL_HPP__

#include "deepMarMobileNet_net.hpp"

#include <memory>
#include <utility>
#include <iostream>

#include <abi/consumer.hpp>

namespace glasssix::rifleman
{
    inline constexpr exposing::utf8_string_view yolov5_deepMarMobileNet_net_qualified_name{ u8"g6.rifleman.deepMarMobileNet_net" };

    class deepMarMobileNet_net_internal;

    class deepMarMobileNet_net_impl : public exposing::implements<deepMarMobileNet_net_impl, deepMarMobileNet_net>, public exposing::make_external_qualified_name<yolov5_deepMarMobileNet_net_qualified_name>
    {
    public:
        deepMarMobileNet_net_impl();
        ~deepMarMobileNet_net_impl();

        void init(exposing::param_string deepMarMobileNet_racy_path, std::int32_t device);
        void init(exposing::param_string deepMarMobileNet_phai, exposing::param_string deepMarMobileNet_racy_path, std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> deepMarMobileNet_phai, exposing::param_string deepMarMobileNet_racy_path, std::int32_t device);

        exposing::param_string version() const;

        exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order);

    private:
        
        std::unique_ptr<deepMarMobileNet_net_internal> impl_;
    };
}

#endif
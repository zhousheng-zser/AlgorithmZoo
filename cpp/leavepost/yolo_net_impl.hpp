#ifndef __YOLO_NET_IMPL_HPP__
#define __YOLO_NET_IMPL_HPP__

#include "yolo_net.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::leavepost
{
    inline constexpr exposing::utf8_string_view leavepost_yolo_net_qualified_name{u8"g6.leavepost.yolo_net"};

    class yolo_net_internal;

    class yolo_net_impl : public exposing::implements<yolo_net_impl, yolo_net>, public exposing::make_external_qualified_name<leavepost_yolo_net_qualified_name>
    {
    public:
        yolo_net_impl();
        ~yolo_net_impl();
		void init(const exposing::param_string& str_params);
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
        exposing::param_string version() const ;
        exposing::param_vector<box_info>  detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y,
         int roi_width, int roi_height,const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const;
   
    private:
        std::unique_ptr<yolo_net_internal> impl_;
    };
}

#endif
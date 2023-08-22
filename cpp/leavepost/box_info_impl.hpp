#ifndef __LEAVEPOST_IMPL_HPP__
#define __LEAVEPOST_IMPL_HPP__

#include "box_info.hpp"
#include "yolo_net_internal.hpp"

#include <abi/consumer.hpp>

namespace glasssix::leavepost
{
    inline constexpr exposing::utf8_string_view leavepost_box_info_qualified_name{u8"g6.leavepost.hatInfo"};

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<leavepost_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal &internal);
        ~box_info_impl();

        float x() const;
		float y() const;
		float width() const;
		float height() const;
		float confidence() const;
		float label() const;

    private:
        box_info_internal internal_;
    };
}

#endif
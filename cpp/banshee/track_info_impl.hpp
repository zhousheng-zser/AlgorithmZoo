#ifndef __TRACK_INFO_IMPL_HPP__
#define __TRACK_INFO_IMPL_HPP__

#include "track_info.hpp"
#include "kcf_tracker_internal.hpp"

namespace glasssix::banshee
{
    inline constexpr exposing::utf8_string_view banshee_track_info_qualified_name{u8"g6.banshee.trackInfo"};

    class track_info_impl : public exposing::implements<track_info_impl, track_info>, public exposing::make_external_qualified_name<banshee_track_info_qualified_name>
    {
    public:
        track_info_impl();
        track_info_impl(const track_info_internal &internal);
        ~track_info_impl();

        float x() const;
        float y() const;
        float width() const;
        float height() const;
        float prob() const;

    private:
        track_info_internal internal_;
    };
}
#endif
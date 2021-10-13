#ifndef KCF_TRACKER_IMPL_H
#define KCF_TRACKER_IMPL_H

#include "kcf_tracker.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::banshee
{
    inline constexpr exposing::utf8_string_view banshee_kcf_tracker_qualified_name{u8"g6.banshee.kcf_tracker"};

    class kcf_tracker_internal;

    class kcf_tracker_impl : public exposing::implements<kcf_tracker_impl, kcf_tracker>, public exposing::make_external_qualified_name<banshee_kcf_tracker_qualified_name>
    {
    public:
        kcf_tracker_impl();
        ~kcf_tracker_impl();
        void init_trace(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height);
        track_info update(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height) const;
        exposing::param_string version() const;

    private:
        std::unique_ptr<kcf_tracker_internal> impl_;
    };
}

#endif
#pragma once

#include "gate_status.hpp"

#include <memory>

#include <abi/consumer.hpp>
#include <abi/param_span.hpp>

namespace glasssix::pump_gate_status
{
    inline constexpr exposing::utf8_string_view pump_gate_status_gate_status_qualified_name{u8"g6.pump_gate_status.gate_status"};

    class gate_status_internal;

    class gate_status_impl : public exposing::implements<gate_status_impl, gate_status>, public exposing::make_external_qualified_name<pump_gate_status_gate_status_qualified_name>
    {
    public:
        gate_status_impl();
        ~gate_status_impl();

        void init(std::int32_t device);
        // void init(const exposing::param_vector<std::int32_t>& hsvs);
        void init(std::int32_t model_type, const exposing::param_string &racy_path, std::int32_t device, bool use_int8);
        void init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string &racy_path, std::int32_t device);
        exposing::param_string version() const;
        exposing::param_vector<exposing::param_vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const;

        std::int32_t detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int yellow_hsv_lower, int yellow_hsv_upper, int gray_hsv_lower, int gray_hsv_upper, 
            const exposing::param_vector<int>& rois,
            const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const;

    private:
        std::unique_ptr<gate_status_internal> impl_;
    };
}

#pragma once

#include "classify_code.hpp"
#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::vehicle
{
    inline constexpr exposing::utf8_string_view vehicle_classify_code_qualified_name{ u8"g6.vehicle.classify_code" };

    class classify_code_internal;

    class classify_code_impl : public exposing::implements<classify_code_impl, classify_code>, public exposing::make_external_qualified_name<vehicle_classify_code_qualified_name>
    {
    public:
        classify_code_impl();
        ~classify_code_impl();
        void init(const exposing::param_string& model_directory, std::int32_t device);
        exposing::param_vector<vehicle::box_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_width, std::int32_t roi_height, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi);
        exposing::param_string version() const;

    private:
        std::unique_ptr<classify_code_internal> impl_;
    };
}

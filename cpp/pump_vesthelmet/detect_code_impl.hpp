#pragma once

#include "detect_code.hpp"
#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::pump_vesthelmet
{
    inline constexpr exposing::utf8_string_view pump_vesthelmet_detect_code_qualified_name{ u8"g6.pump_vesthelmet.detect_code" };

    class detect_code_internal;

    class detect_code_impl : public exposing::implements<detect_code_impl, detect_code>, public exposing::make_external_qualified_name<pump_vesthelmet_detect_code_qualified_name>
    {
    public:
        detect_code_impl();
        ~detect_code_impl();
        void init(const exposing::param_string& str_params);
        exposing::param_string execute(exposing::param_hash_map<exposing::param_string, exposing::unknown_object> input_params_map);
        exposing::param_vector<pump_vesthelmet::box_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi);
        exposing::param_string version() const;

    private:
        std::unique_ptr<detect_code_internal> impl_;
    };
}

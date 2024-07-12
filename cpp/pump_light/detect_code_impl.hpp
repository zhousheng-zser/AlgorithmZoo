#ifndef __DETECT_CODE_IMPL_HPP__
#define __DETECT_CODE_IMPL_HPP__

#include "detect_code.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::pump_light
{
    inline constexpr exposing::utf8_string_view pump_light_detect_code_qualified_name{ u8"g6.pump_light.detect_code" };

    class detect_code_internal;

    class detect_code_impl : public exposing::implements<detect_code_impl, detect_code>, public exposing::make_external_qualified_name<pump_light_detect_code_qualified_name>
    {
    public:
        detect_code_impl();
        ~detect_code_impl();

        void init(const exposing::param_string& str_params);

        exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
        exposing::param_string version() const;

        pump_light::box_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width,
           const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const;

    private:

        std::unique_ptr<detect_code_internal> impl_;
    };
}

#endif
#pragma once

#include "detect_code.hpp"
#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::fighting
{
    inline constexpr exposing::utf8_string_view fighting_detect_code_qualified_name{ u8"g6.fighting.detect_code" };

    class detect_code_internal;

    class detect_code_impl : public exposing::implements<detect_code_impl, detect_code>, public exposing::make_external_qualified_name<fighting_detect_code_qualified_name>
    {
    public:
        detect_code_impl();
        ~detect_code_impl();
        
        void init(const exposing::param_string& str_params);

        exposing::param_string version() const;

        exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);

    private:
        std::unique_ptr<detect_code_internal> impl_;
    };
}

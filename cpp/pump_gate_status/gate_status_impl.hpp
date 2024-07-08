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

        void init(const exposing::param_string& str_params);

        exposing::param_string version() const;

        exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);

    private:
        std::unique_ptr<gate_status_internal> impl_;
    };
}

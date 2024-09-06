#pragma once

#include "work_status.hpp"

#include <memory>

#include <abi/consumer.hpp>
#include <abi/param_span.hpp>

namespace glasssix::pump_work_status
{
    inline constexpr exposing::utf8_string_view pump_work_status_work_status_qualified_name{ u8"g6.pump_work_status.detect_code" };

    class work_status_internal;

    class work_status_impl : public exposing::implements<work_status_impl, work_status>, public exposing::make_external_qualified_name<pump_work_status_work_status_qualified_name>
    {
    public:
        work_status_impl();
        ~work_status_impl();

        void init(const exposing::param_string& str_params);

        exposing::param_string version() const;

        exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
    };
}

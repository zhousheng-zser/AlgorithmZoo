#ifndef __CLASSIFY_CODE_IMPL_HPP__
#define __CLASSIFY_CODE_IMPL_HPP__

#include "classify_code.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::refvest
{
    inline constexpr exposing::utf8_string_view refvest_classify_code_qualified_name{ u8"g6.refvest.detect_code" };

    class classify_code_internal;

    class classify_code_impl : public exposing::implements<classify_code_impl, classify_code>, public exposing::make_external_qualified_name<refvest_classify_code_qualified_name>
    {
    public:
        classify_code_impl();
        ~classify_code_impl();

        void init(const exposing::param_string& str_params);

        exposing::param_string version() const;

        exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);

    private:

        std::unique_ptr<classify_code_internal> impl_;
    };
}

#endif
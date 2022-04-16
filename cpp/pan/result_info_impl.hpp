#pragma once

#include "result_info.hpp"


namespace glasssix::pan
{
    inline constexpr exposing::utf8_string_view pan_result_info_qualified_name{ u8"g6.pan.resultInfo" };

    class result_info_impl : public exposing::implements<result_info_impl, result_info>, public exposing::make_external_qualified_name<pan_result_info_qualified_name>
    {
    public:
        result_info_impl();
        result_info_impl(const result_info_internal& internal);
        ~result_info_impl();

        // coordinates: x,y,xx,yy
        exposing::param_vector<exposing::param_vector<float>> coordinates() const;
        exposing::param_vector<float> conf() const;
        exposing::param_vector<exposing::param_string> cls() const;
        exposing::param_vector<int> num() const;
        exposing::param_vector<float> speed() const;

    private:
        result_info_internal internal_;
    };
}
#pragma once

#include "result_info.hpp"


namespace glasssix::yolov5
{
    inline constexpr exposing::utf8_string_view yolov5_result_info_qualified_name{ u8"g6.yolov5.resultInfo" };

    class result_info_impl : public exposing::implements<result_info_impl, result_info>, public exposing::make_external_qualified_name<yolov5_result_info_qualified_name>
    {
    public:
        result_info_impl();
        result_info_impl(const result_info_internal& internal);
        ~result_info_impl();

        exposing::param_vector<exposing::param_vector<float>> coordinates() const;
        exposing::param_vector<float> conf() const;
        exposing::param_vector<exposing::param_string> cls() const;

    private:
        result_info_internal internal_;
    };
}
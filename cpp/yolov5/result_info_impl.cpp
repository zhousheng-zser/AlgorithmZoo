#include "result_info_impl.hpp"

namespace glasssix::yolov5
{
    result_info_impl::result_info_impl()
    {
    }

    result_info_impl::result_info_impl(const result_info_internal& internal) : internal_(internal)
    {
    }

    result_info_impl::~result_info_impl()
    {
    }

    exposing::param_vector<exposing::param_vector<float>> result_info_impl::coordinates() const
    {
        return internal_.coordinates;
    }

    exposing::param_vector<float> result_info_impl::conf() const
    {
        return internal_.conf;
    }
    exposing::param_vector<exposing::param_string> result_info_impl::cls() const
    {
        return internal_.cls;
    }
}
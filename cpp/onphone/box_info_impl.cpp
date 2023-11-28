#include "box_info_impl.hpp"

namespace glasssix::onphone
{
    box_info_impl::box_info_impl() {}

    box_info_impl::box_info_impl(const box_info_internal &internal) : internal_(internal) {}

    box_info_impl::~box_info_impl() {}

    std::int32_t box_info_impl::x1()
    {
        return internal_.x1;
    }

    std::int32_t box_info_impl::x2()
    {
        return internal_.x2;
    }

    std::int32_t box_info_impl::y1()
    {
        return internal_.y1;
    }

    std::int32_t box_info_impl::y2()
    {
        return internal_.y2;
    }

    float box_info_impl::category()
    {
        return internal_.category;
    }

    float box_info_impl::confidence()
    {
        return internal_.confidence;
    }

    exposing::param_vector<std::int32_t> box_info_impl::phonelocal_list()
    {
        return internal_.phonelocal_list;
    }

    exposing::param_vector<float> box_info_impl::phonescore_list()
    {
        return internal_.phonescore_list;
    }

}

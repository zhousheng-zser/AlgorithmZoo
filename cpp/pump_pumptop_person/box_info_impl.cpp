#include "box_info_impl.hpp"

namespace glasssix::pump_pumptop_person
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

    float box_info_impl::score()
    {
        return internal_.score;
    }

    std::int32_t box_info_impl::category()
    {
        return internal_.category;
    }

    exposing::param_vector<std::int32_t> box_info_impl::pump()
    {
        return internal_.pump;
    }

}

#include "box_info_impl.hpp"

namespace glasssix::pump_weld
{
    box_info_impl::box_info_impl() {}

    box_info_impl::box_info_impl(const box_info_internal &internal) : internal_(internal) {}

    box_info_impl::~box_info_impl() {}

    exposing::param_vector<std::int32_t> box_info_impl::weldlocal_list() const
    {
        return internal_.weldlocal_list;
    }

    std::int32_t box_info_impl::can_x1()
    {
        return internal_.can_x1;
    }

    std::int32_t box_info_impl::can_x2()
    {
        return internal_.can_x2;
    }

    std::int32_t box_info_impl::can_y1()
    {
        return internal_.can_y1;
    }

    std::int32_t box_info_impl::can_y2()
    {
        return internal_.can_y2;
    }

    float box_info_impl::score()
    {
        return internal_.score;
    }

    std::int32_t box_info_impl::category()
    {
        return internal_.category;
    }

}

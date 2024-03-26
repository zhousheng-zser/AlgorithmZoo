#include "box_info_impl.hpp"

namespace glasssix::vehicle
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

    std::int32_t box_info_impl::x3()
    {
        return internal_.x3;
    }

    std::int32_t box_info_impl::x4()
    {
        return internal_.x4;
    }

    std::int32_t box_info_impl::x5()
    {
        return internal_.x5;
    }

    std::int32_t box_info_impl::y1()
    {
        return internal_.y1;
    }

    std::int32_t box_info_impl::y2()
    {
        return internal_.y2;
    }

    std::int32_t box_info_impl::y3()
    {
        return internal_.y3;
    }

    std::int32_t box_info_impl::y4()
    {
        return internal_.y4;
    }

    std::int32_t box_info_impl::y5()
    {
        return internal_.y5;
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

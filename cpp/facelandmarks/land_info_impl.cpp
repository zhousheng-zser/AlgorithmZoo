#include "land_info_impl.hpp"

namespace glasssix::facelandmarks
{
    land_info_impl::land_info_impl() {}

    land_info_impl::land_info_impl(const land_info_internal &internal) : internal_(internal) {}

    land_info_impl::~land_info_impl() {}

    exposing::param_vector<exposing::param_pair<float,float>> land_info_impl::pts()
    {
        return internal_.pts;
    }

    float land_info_impl::score()
    {
        return internal_.score;
    }

}

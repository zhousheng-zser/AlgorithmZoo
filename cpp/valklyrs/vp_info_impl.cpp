#include "vp_info_impl.hpp"

namespace glasssix::valklyrs
{
    vp_info_impl::vp_info_impl()
    {
    }

    vp_info_impl::vp_info_impl(const vp_info_internal &internal) : internal_(internal)
    {

    }

    vp_info_impl::~vp_info_impl()
    {
    }

    exposing::param_vector<float> vp_info_impl::coordinates() const
    {
        return internal_.coordinates;
    }

    exposing::param_vector<exposing::param_string> vp_info_impl::attributes() const
    {
        return internal_.attributes;
    }
}
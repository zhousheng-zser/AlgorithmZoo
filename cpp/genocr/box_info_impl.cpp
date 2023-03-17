#include "box_info_impl.hpp"

namespace glasssix::genocr
{
    box_info_impl::box_info_impl()
    {
    }

    box_info_impl::box_info_impl(const box_info_internal &internal) : internal_(internal)
    {
    }

    box_info_impl::~box_info_impl()
    {
    }

    exposing::param_vector<float> box_info_impl::location() const
	{
		return internal_.location;
	}

	exposing::param_vector<exposing::param_string> box_info_impl::strinfos() const
	{
		return internal_.strinfos;
	}

	float box_info_impl::angle() const
	{
		return internal_.angle;
	}
}
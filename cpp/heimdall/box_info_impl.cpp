#include "box_info_impl.hpp"

namespace glasssix::heimdall
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

	exposing::param_string box_info_impl::strinfo() const
	{
		return internal_.strinfo;
	}

	float box_info_impl::angle() const
	{
		return internal_.angle;
	}
}
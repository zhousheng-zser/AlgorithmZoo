#include "box_info_impl.hpp"

namespace glasssix::ring
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

	exposing::param_vector<std::uint8_t> box_info_impl::cut_roi() const
	{
		return internal_.cut_roi;
	}

	std::int32_t box_info_impl::cut_roi_width() const
	{
		return internal_.cut_roi_width;
	}

	std::int32_t box_info_impl::cut_roi_height() const
	{
		return internal_.cut_roi_height;
	}
}
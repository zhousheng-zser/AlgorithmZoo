#include "box_info_impl.hpp"

namespace glasssix::plate
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

	exposing::param_string box_info_impl::strinfos() const
	{
		return internal_.strinfos;
	}

    exposing::param_vector<std::uint8_t> box_info_impl::aligned_images() const
    {
        return internal_.aligned_images;
    }

}
#include "box_info_impl.hpp"

namespace glasssix::leavepost
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

    float box_info_impl::x() const
	{
		return internal_.rect.x;
	}

	float box_info_impl::y() const
	{
		return internal_.rect.y;
	}

	float box_info_impl::width() const
	{
		return internal_.rect.width;
	}

	float box_info_impl::height() const
	{
		return internal_.rect.height;
	}

    float box_info_impl::confidence() const
	{
		return internal_.confidence;
	}

    float box_info_impl::label() const
	{
		return internal_.label;
	}
}
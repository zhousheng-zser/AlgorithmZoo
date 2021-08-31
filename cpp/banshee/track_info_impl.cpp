#include "track_info_impl.hpp"

namespace glasssix::banshee
{
    track_info_impl::track_info_impl()
    {
    }

    track_info_impl::track_info_impl(const track_info_internal &internal) : internal_(internal)
    {
    }

    track_info_impl::~track_info_impl()
    {
    }

    float track_info_impl::x() const
	{
		return internal_.x;
	}

	float track_info_impl::y() const
	{
		return internal_.y;
	}

	float track_info_impl::width() const
	{
		return internal_.width;
	}

    float track_info_impl::height() const
	{
		return internal_.height;
	}

    float track_info_impl::prob() const
	{
		return internal_.prob;
	}
}
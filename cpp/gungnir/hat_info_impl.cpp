#include "hat_info_impl.hpp"

namespace glasssix::gungnir
{
    hat_info_impl::hat_info_impl()
    {
    }

    hat_info_impl::hat_info_impl(const hat_info_internal &internal) : internal_(internal)
    {
    }

    hat_info_impl::~hat_info_impl()
    {
    }

    float hat_info_impl::x() const
	{
		return internal_.rect.x;
	}

	float hat_info_impl::y() const
	{
		return internal_.rect.y;
	}

	float hat_info_impl::width() const
	{
		return internal_.rect.width;
	}

	float hat_info_impl::height() const
	{
		return internal_.rect.height;
	}

    float hat_info_impl::prob() const
	{
		return internal_.prob;
	}

    float hat_info_impl::label() const
	{
		return internal_.label;
	}
}
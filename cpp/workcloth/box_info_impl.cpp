#include "box_info_impl.hpp"

namespace glasssix::workcloth
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()	= default;

	int box_info_impl::x1() const
	{
		return internal_.x1;
	}

    int box_info_impl::y1() const
	{
		return internal_.y1;
	}

    int box_info_impl::x2() const
	{
		return internal_.x2;
	}

    int box_info_impl::y2() const
	{
		return internal_.y2;
	}

    bool box_info_impl::color_pure() const
	{
		return internal_.color_pure;
	}

    float box_info_impl::color_conf() const
	{
		return internal_.color_conf;
	}

    float box_info_impl::score() const
	{
		return internal_.score;
	}

	int box_info_impl::category() const
	{
		return internal_.category;
	}
}
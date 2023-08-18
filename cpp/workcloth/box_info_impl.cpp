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

    int box_info_impl::is_sleeve() const
	{
		return internal_.is_sleeve;
	}

    int box_info_impl::color_type() const
	{
		return internal_.color_type;
	}

    float box_info_impl::color_conf() const
	{
		return internal_.color_conf;
	}

}
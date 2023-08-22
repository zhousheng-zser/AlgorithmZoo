#include "box_info_impl.hpp"

namespace glasssix::needledash
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()
	{
	}

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

	exposing::param_string box_info_impl::strinfo() const
	{
		return internal_.strinfo;
	}
}
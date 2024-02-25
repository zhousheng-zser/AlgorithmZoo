#include "box_info_impl.hpp"

namespace glasssix::pump_hoisting
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()
	= default;

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

		int box_info_impl::x3() const
	{
		return internal_.x3;
	}

    int box_info_impl::y3() const
	{
		return internal_.y3;
	}

    int box_info_impl::x4() const
	{
		return internal_.x4;
	}

    int box_info_impl::y4() const
	{
		return internal_.y4;
	}

	int box_info_impl::category() const
	{
		return internal_.category;
	}

	float box_info_impl::confidence() const
	{
		return internal_.confidence;
	}

	
}
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

	exposing::param_vector<int> box_info_impl::up_rgb() const
	{
		return internal_.up_rgb;
	}

	exposing::param_vector<int> box_info_impl::lw_rgb() const
	{
		return internal_.lw_rgb;
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
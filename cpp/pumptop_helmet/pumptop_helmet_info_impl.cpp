#include "pumptop_helmet_info_impl.hpp"

namespace glasssix::pumptop_helmet
{
	pumptop_helmet_info_impl::pumptop_helmet_info_impl()
	{
	}

	pumptop_helmet_info_impl::pumptop_helmet_info_impl(const pumptop_helmet::pumptop_helmet_info_internal& internal) : internal_(internal)
	{
	}
	pumptop_helmet_info_impl::~pumptop_helmet_info_impl()
	{
	}

	int pumptop_helmet_info_impl::x1() const
	{
		return internal_.x1;
	}

	int pumptop_helmet_info_impl::y1() const
	{
		return internal_.y1;
	}

	int pumptop_helmet_info_impl::x2() const
	{
		return internal_.x2;
	}

	int pumptop_helmet_info_impl::y2() const
	{
		return internal_.y2;
	}

	int pumptop_helmet_info_impl::category() const
	{
		return internal_.category;
	}

	float pumptop_helmet_info_impl::score() const
	{
		return internal_.score;
	}

	float pumptop_helmet_info_impl::helmet_score() const
	{
		return internal_.helmet_score;
	}
}

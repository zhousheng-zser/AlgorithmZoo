#include "box_info_impl.hpp"

namespace glasssix::helmet
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

	int box_info_impl::category() const
	{
		return internal_.category;
	}

	exposing::param_string box_info_impl::version() const
	{
		return internal_.version;
	}
	
}
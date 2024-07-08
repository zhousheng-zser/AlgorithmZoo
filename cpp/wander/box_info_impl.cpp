#include "box_info_impl.hpp"

namespace glasssix::wander
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()
	= default;

	int box_info_impl::id() const
	{
		return internal_.id;
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

	float box_info_impl::confidence() const
	{
		return internal_.confidence;
	}

	float box_info_impl::cosine_similarity() const
	{
		return internal_.cosine_similarity;
	}

	double box_info_impl::first_show_time() const
	{
		return internal_.first_show_time;
	}

	double box_info_impl::last_show_time() const
	{
		return internal_.last_show_time;
	}


}
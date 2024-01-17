#include "box_info_impl.hpp"

namespace glasssix::head
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

    float box_info_impl::score() const
	{
		return internal_.score;
	}

	int box_info_impl::category() const
	{
		return internal_.category;
	}

	//exposing::param_vector<float> box_info_impl::key_points() const
	//{
	//	return internal_.key_points;
	//}

	void box_info_impl::set_x1(int input)
	{
		internal_.x1 = input;
	}
	void box_info_impl::set_y1(int input)
	{
		internal_.y1 = input;
	}
	void box_info_impl::set_x2(int input)
	{
		internal_.x2 = input;
	}
	void box_info_impl::set_y2(int input)
	{
		internal_.y2 = input;
	}
	void box_info_impl::set_score(float input)
	{
		internal_.score = input;
	}
	void box_info_impl::set_category(int input)
	{
		internal_.category = input;
	}

}
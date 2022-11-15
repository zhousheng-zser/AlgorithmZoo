#include "box_info_impl.hpp"

namespace glasssix::plate
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

	float box_info_impl::x() const
	{
		return internal_.rect.x;
	}

	float box_info_impl::y() const
	{
		return internal_.rect.y;
	}

	float box_info_impl::width() const
	{
		return internal_.rect.w;
	}

	float box_info_impl::height() const
	{
		return internal_.rect.h;
	}

	exposing::param_string box_info_impl::strinfos() const
	{
		return internal_.strinfos;
	}

	exposing::param_vector<std::uint8_t> box_info_impl::aligned_images() const
	{
		return internal_.aligned_images;
	}

	void box_info_impl::set_x(float input)
	{
		internal_.rect.x = input;
	}
	void box_info_impl::set_y(float input)
	{
		internal_.rect.y = input;
	}
	void box_info_impl::set_width(float input)
	{
		internal_.rect.w = input;
	}
	void box_info_impl::set_height(float input)
	{
		internal_.rect.h = input;
	}
	void box_info_impl::set_strinfos(exposing::param_string input)
	{
		internal_.strinfos = input;
	}
	void box_info_impl::set_aligned_images(exposing::param_vector<std::uint8_t> input)
	{
		internal_.aligned_images = input;
	}

}
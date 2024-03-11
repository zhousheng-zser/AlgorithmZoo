#include "box_info_impl.hpp"

namespace glasssix::pump_light
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()
	= default;
	
	float box_info_impl::red_ratio() const
	{
		return internal_.red_ratio;
	}

	float box_info_impl::white_ratio() const
	{
		return internal_.white_ratio;
	}

	float box_info_impl::orange_ratio() const
	{
		return internal_.orange_ratio;
	}

	float box_info_impl::grey_ratio() const
	{
		return internal_.grey_ratio;
	}

	bool box_info_impl::light_status() const
	{
		return internal_.light_status;
	}

	exposing::param_string box_info_impl::version() const
	{
		return internal_.version;
	}
	
}
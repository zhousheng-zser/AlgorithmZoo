#include "box_info_impl.hpp"

namespace glasssix::subway_anomaly
{
	box_info_impl::box_info_impl()
	{
	}

	box_info_impl::box_info_impl(const box_info_internal& internal) : internal_(internal)
	{
	}

	box_info_impl::~box_info_impl()
	= default;

	float box_info_impl::score() const
	{
		return internal_.score;
	}

	bool box_info_impl::anomaly_status() const
	{
		return internal_.anomaly_status;
	}

	exposing::param_string box_info_impl::version() const
	{
		return internal_.version;
	}
	
}
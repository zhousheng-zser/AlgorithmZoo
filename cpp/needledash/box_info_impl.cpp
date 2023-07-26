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

	exposing::param_string box_info_impl::strinfo() const
	{
		return internal_.strinfo;
	}
}
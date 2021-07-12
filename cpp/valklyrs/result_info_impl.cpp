#include "result_info_impl.hpp"
#include "vp_info_impl.hpp"

namespace glasssix::valklyrs
{
    result_info_impl::result_info_impl()
    {
    }

    result_info_impl::result_info_impl(const result_info_internal &internal) : internal_(internal)
    {
    }

    result_info_impl::~result_info_impl()
    {
    }

    exposing::param_vector<vp_info> result_info_impl::vehicle_list() const
	{
		return internal_.vehicle_list;
	}

	exposing::param_vector<vp_info> result_info_impl::person_list() const
	{
		return internal_.person_list;
	}
}
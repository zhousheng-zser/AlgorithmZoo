#include "search_result_impl.hpp"
#include "database_search_result.hpp"

namespace glasssix::irisviel
{
	search_result_impl::search_result_impl(const database_search_result& result) : impl_{ std::make_unique<database_search_result>(result) }
	{
	}

	search_result_impl::~search_result_impl()
	{
	}

	float search_result_impl::similarity() const
	{
		return impl_->similarity;
	}

	exposing::param_string search_result_impl::key() const
	{
		return exposing::to_param_string(impl_->data->key());
	}

	exposing::param_vector<float> search_result_impl::feature() const
	{
		return exposing::make_param_vector<float>(impl_->data->feature());
	}
}

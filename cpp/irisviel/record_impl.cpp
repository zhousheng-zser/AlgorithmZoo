#include "record_impl.hpp"
#include "database_record.hpp"

namespace glasssix::irisviel
{
	record_impl::record_impl(int dimension) : impl_{ database_record::create(dimension) }
	{
	}

	std::int32_t record_impl::dimension() const
	{
		return impl_->dimension();
	}

	exposing::param_string record_impl::key() const
	{
		return exposing::to_param_string(impl_->key());
	}

	void record_impl::key(const exposing::param_string& value) const
	{
		impl_->key(exposing::to_narrow_string(value));
	}

	exposing::param_vector<float> record_impl::feature() const
	{
		return exposing::make_param_vector<float>(impl_->feature());
	}

	void record_impl::feature(exposing::param_span<const float> value) const
	{
		impl_->feature(value);
	}
}

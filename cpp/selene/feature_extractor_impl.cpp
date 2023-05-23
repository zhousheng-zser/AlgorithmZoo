#include "feature_extractor_impl.hpp"
#include "feature_extractor_internal.hpp"

namespace glasssix::selene
{
	feature_extractor_impl::feature_extractor_impl()
	{
	}

	feature_extractor_impl::~feature_extractor_impl()
	{
	}

	void feature_extractor_impl::init(const exposing::param_string& models_directory, int model_type, std::int32_t device, bool use_int8)
	{
		impl_ = std::make_unique<feature_extractor_internal>(exposing::to_narrow_string(models_directory), model_type, device, use_int8);
	}

	std::int32_t feature_extractor_impl::get_model_type() const
	{
		return impl_->get_model_type();
	}

	exposing::param_string feature_extractor_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<exposing::param_vector<float>> feature_extractor_impl::get(exposing::param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const
	{
		auto native_result = impl_->get(bitmaps, count, order);
		auto result = exposing::make_param_vector<float, 2>();

		for (const auto& item : native_result)
		{
			result.push_back(exposing::make_param_vector<float>(item));
		}

		return result;
	}
}

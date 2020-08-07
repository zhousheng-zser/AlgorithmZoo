#include "feature_extractor_impl.hpp"
#include "feature_extractor_native.hpp"

namespace glasssix::cassius
{
	feature_extractor_impl::feature_extractor_impl() : feature_extractor_impl{ -1 }
	{
	}

	feature_extractor_impl::feature_extractor_impl(int device) : impl_{ new feature_extractor_native{ device } }
	{
	}

	feature_extractor_impl::~feature_extractor_impl()
	{
		if (impl_)
		{
			delete impl_;
		}
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

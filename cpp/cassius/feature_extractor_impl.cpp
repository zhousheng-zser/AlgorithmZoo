#include "feature_extractor_impl.hpp"
#include "feature_extractor_internal.hpp"

namespace glasssix::cassius
{
	feature_extractor_impl::feature_extractor_impl()
	{
	}

	feature_extractor_impl::~feature_extractor_impl()
	{
	}

	void feature_extractor_impl::init(const exposing::param_string& racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<feature_extractor_internal>(exposing::to_narrow_string(racy_path), device);
	}

	void feature_extractor_impl::init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string& racy_path, std::int32_t device)
	{
		std::vector<std::string> phai_internal(phai.size());

		std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<feature_extractor_internal>(phai_internal, exposing::to_narrow_string(racy_path), device);
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

#pragma once

#include "feature_extractor.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::gaius
{
	inline constexpr exposing::utf8_string_view gaius_feature_extractor_qualified_name{ u8"g6.gaius.featureExtractor" };

	class feature_extractor_internal;

	class feature_extractor_impl : public exposing::implements<feature_extractor_impl, feature_extractor>, public exposing::make_external_qualified_name<gaius_feature_extractor_qualified_name>
	{
	public:
		feature_extractor_impl();
		~feature_extractor_impl();
		void init(const exposing::param_string& racy_path, const exposing::param_string& mask_racy_path, std::int32_t device, bool use_int8);
		void init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string& racy_path, exposing::param_span<const exposing::param_string> mask_phai, const exposing::param_string& mask_racy_path, std::int32_t device);
		exposing::param_string version() const;
		exposing::param_vector<exposing::param_vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order, bool has_mask) const;
	private:
		std::unique_ptr<feature_extractor_internal> impl_;
	};
}

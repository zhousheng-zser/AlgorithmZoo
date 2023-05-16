#pragma once

#include "feature_extractor.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::selene
{
	inline constexpr exposing::utf8_string_view selene_feature_extractor_qualified_name{ u8"g6.selene.featureExtractor" };

	class feature_extractor_internal;

	class feature_extractor_impl : public exposing::implements<feature_extractor_impl, feature_extractor>, public exposing::make_external_qualified_name<selene_feature_extractor_qualified_name>
	{
	public:
		feature_extractor_impl();
		~feature_extractor_impl();
		void init(const exposing::param_string& models_directory, int model_type, std::int32_t device, bool use_int8);
		std::int32_t get_model_type() const;
		exposing::param_string version() const;
		exposing::param_vector<exposing::param_vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const;
	private:
		std::unique_ptr<feature_extractor_internal> impl_;
	};
}

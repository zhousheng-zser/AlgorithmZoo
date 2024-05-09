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
		void init(const exposing::param_string& str_params);
		exposing::param_string version() const;
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
		std::int32_t get_model_type() const;
	private:
		std::unique_ptr<feature_extractor_internal> impl_;
	};
}

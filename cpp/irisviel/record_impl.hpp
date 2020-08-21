#pragma once

#include "record.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	inline constexpr exposing::utf8_string_view irisviel_record_qualified_name{ u8"g6.irisviel.record" };

	struct database_record;

	class record_impl : public exposing::implements<record_impl, record>, public exposing::make_external_qualified_name<irisviel_record_qualified_name>
	{
	public:
		void init(std::int32_t dimension);
		std::int32_t dimension() const;
		exposing::param_string key() const;
		void key(const exposing::param_string& value) const;
		exposing::param_vector<float> feature() const;
		void feature(const exposing::param_vector<float>& value) const;
		void feature(exposing::param_span<const float> value) const;
	private:
		std::shared_ptr<database_record> impl_;
	};
}

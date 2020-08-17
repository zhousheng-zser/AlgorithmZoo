#pragma once

#include "record.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct database_record;

	class record_impl : public exposing::implements<record_impl, record>
	{
	public:
		record_impl(int dimension);
		std::int32_t dimension() const;
		exposing::param_string key() const;
		void key(const exposing::param_string& value) const;
		exposing::param_vector<float> feature() const;
		void feature(exposing::param_span<const float> value) const;
	private:
		std::shared_ptr<database_record> impl_;
	};
}

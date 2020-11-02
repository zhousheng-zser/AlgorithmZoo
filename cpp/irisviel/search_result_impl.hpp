#pragma once

#include "search_result.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct database_search_result;

	class search_result_impl : public exposing::implements<search_result_impl, search_result>
	{
	public:
		search_result_impl(const database_search_result& result);
		~search_result_impl();
		float similarity() const;
		exposing::param_string key() const;
		exposing::param_vector<float> feature() const;
	private:
		std::unique_ptr<database_search_result> impl_;
	};
}

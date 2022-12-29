#pragma once

#include "irisviel_types.hpp"
#include "database_feature_observer.hpp"
#include "database_record.hpp"
#include "database_search_result.hpp"

#include <vector>
#include <optional>
#include <string_view>

namespace glasssix::irisviel
{
	struct feature_searcher
	{
		virtual ~feature_searcher() = default;
		virtual int dimension() const noexcept = 0;
		virtual void build_cache() const = 0;
		virtual void save_cache(std::string_view path) const = 0;
		virtual void load_cache(std::string_view path) const = 0;
		virtual void current_data(const std::vector<database_feature_observer::feature>& data) noexcept = 0;
		virtual std::vector<std::vector<database_search_result>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const = 0;
		virtual bool add(database_record &record)  = 0;
		virtual std::vector<bool> remove(std::vector<std::string>& keys)  = 0;
		virtual std::vector<bool> update(const std::vector<std::shared_ptr<database_record>>& records)  const = 0;
	};
}

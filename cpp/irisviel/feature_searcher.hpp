#pragma once

#include "irisviel_types.hpp"

#include <vector>
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
		virtual void current_data(const std::vector<const float*>& data) noexcept = 0;
		virtual vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, std::uint32_t top_k) const = 0;
	};
}

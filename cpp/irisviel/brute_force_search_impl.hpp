#pragma once

#include "feature_searcher.hpp"
#include "database_feature_observer.hpp"

#include <memory>

namespace glasssix::irisviel
{
	class brute_force_search_impl : public feature_searcher
	{
	public:
		class impl;

		brute_force_search_impl(int dimension);
		virtual ~brute_force_search_impl();
		virtual int dimension() const noexcept override;
		virtual void build_cache() const override;
		virtual void save_cache(std::string_view path) const override;
		virtual void load_cache(std::string_view path) const override;
		virtual void current_data(const std::vector<database_feature_observer::feature>& data) noexcept override;
		virtual vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const override;
	private:
		std::unique_ptr<impl> impl_;
	};
}

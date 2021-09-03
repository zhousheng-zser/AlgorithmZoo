#pragma once

#include "feature_searcher.hpp"

#include <memory>

namespace glasssix::irisviel
{
	class nsg_search_impl : public feature_searcher
	{
	public:
		nsg_search_impl(int dimension);
		virtual ~nsg_search_impl();
		virtual int dimension() const noexcept override;
		virtual void build_cache() const override;
		virtual void save_cache(std::string_view path) const override;
		virtual void load_cache(std::string_view path) const override;
		virtual void current_data(const std::vector<const float*>& data) noexcept override;
		virtual vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const override;
	private:
		class impl;

		std::unique_ptr<impl> impl_;
	};
}

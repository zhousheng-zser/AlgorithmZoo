#pragma once

#include "feature_searcher.hpp"
#include "database_feature_observer.hpp"
#include "database_record.hpp"
#include "database_search_result.hpp"

#include <memory>

namespace glasssix::irisviel
{
	class nsg_search_impl : public feature_searcher
	{
	public:
		nsg_search_impl(int dimension,std::string path);
		virtual ~nsg_search_impl();
		virtual int dimension() const noexcept override;
		virtual void build_cache() const override;
		virtual void save_cache(std::string_view path) const override;
		virtual void load_cache(std::string_view path) const override;
		virtual void current_data(const std::vector<database_feature_observer::feature>& data) noexcept override;
		virtual std::vector<std::vector<database_search_result>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const override;
		virtual void add(database_record &record) override;
		virtual void remove(std::vector<std::string>& keys)  override;
		virtual void update(const std::vector<std::shared_ptr<database_record>>& records)  const override;
	private:
		class impl;

		std::unique_ptr<impl> impl_;
	};
}

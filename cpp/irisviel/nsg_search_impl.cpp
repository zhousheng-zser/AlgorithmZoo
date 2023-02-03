#include "nsg_search_impl.hpp"
#include "irisviel_search.hpp"
#include "feature_searcher_factory.hpp"
#include "face_service_implemention.hpp"

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <functional>

namespace glasssix::irisviel
{
	class nsg_search_impl::impl
	{
	public:
		impl(int dimension) : dimension_{ dimension }, nsg_search_{ dimension }, current_data_{}
		{
		}

		int dimension() const noexcept
		{
			return dimension_;
		}

		void build_cache() const
		{
			if (current_data_)
			{
				std::vector<const float*> data(current_data_->size());

				std::transform(current_data_->begin(), current_data_->end(), data.begin(), [](const database_feature_observer::feature& inner) { return inner.data; });
				nsg_search_.build_graph(data);
				nsg_search_.optimize_graph();
			}
		}

		void save_cache(std::string_view path) const
		{
			nsg_search_.save_graph(path.data());
		}

		void load_cache(std::string_view path) const
		{
			nsg_search_.load_graph(path.data());
		}

		void current_data(const std::vector<database_feature_observer::feature>& data) noexcept
		{
			current_data_ = &data;
		}

		std::vector<std::vector<database_search_result>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k, bool result_has_feature) const
		{
			// This implementation does not support searching by min similarity.
			if (!top_k)
			{
				throw std::invalid_argument{ u8"The top K must be set in the NSG implementation." };
			}
			auto actual_size = top_k ? std::min<std::size_t>(current_data_->size(), *top_k) : current_data_->size();

			vector2d<std::tuple<std::uint32_t, float>> raw_search_result = nsg_search_.search_vector(query_data, actual_size);
			
			std::vector<std::vector<database_search_result>> result;
			for (auto&& item : raw_search_result)
			{
				std::vector<database_search_result> inner;
				for (auto&& [index, similarity] : item)
				{
					if (current_data_->size() == 1)
					{
						index = std::min(index, 0U);
					}
					if (index >= current_data_->size())
					{
						continue;
					}
					// Retrieve the orginal data in the mapping file.
					auto offset = reinterpret_cast<const std::uint8_t*>((*current_data_)[index].data) - database_record::feature_offset(dimension_);
                    auto result_temp = database_record::create(dimension_, const_cast<std::uint8_t*>(offset));
                    if (result_has_feature)
                        inner.emplace_back(database_search_result{ result_temp, similarity });
                    else
                    {
                        auto result = database_record::create(0);
                        result->feature(nullptr);
                        result->key(result_temp->key());
						inner.emplace_back(database_search_result{ result, similarity });
					}
				}
				result.emplace_back(inner);
			}
			return result;
		}
	private:
		int dimension_;
		irisviel_search nsg_search_;
		const std::vector<database_feature_observer::feature>* current_data_;
	};

	nsg_search_impl::nsg_search_impl(int dimension,std::string path) : impl_{ std::make_unique<impl>(dimension) }
	{
	}

	nsg_search_impl::~nsg_search_impl()
	{
	}

	int nsg_search_impl::dimension() const noexcept
	{
		return impl_->dimension();
	}

	void nsg_search_impl::build_cache() const
	{
		impl_->build_cache();
	}

	void nsg_search_impl::save_cache(std::string_view path) const
	{
		impl_->save_cache(path);
	}

	void nsg_search_impl::load_cache(std::string_view path) const
	{
		impl_->load_cache(path);
	}

	void nsg_search_impl::current_data(const std::vector<database_feature_observer::feature>& data) noexcept
	{
		impl_->current_data(data);
	}

	std::vector<std::vector<database_search_result>>  nsg_search_impl::search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k, bool result_has_feature) const
	{
		return impl_->search_vector(query_data, min_similarity, top_k, result_has_feature);
	}

	bool nsg_search_impl::add(database_record& record)
	{
		return false;
	}

	std::vector<bool> nsg_search_impl::remove(std::vector<std::string>& keys)
	{
		return {};
	}

	std::vector<bool> nsg_search_impl::update(const std::vector<std::shared_ptr<database_record>>& records)  const
	{
		return {};
	}
	std::uint64_t nsg_search_impl::count() const
	{
		return {};
	}
	bool nsg_search_impl::contains(std::string_view key) const
	{
		return {};
	}

	namespace
	{
		int register_hint = (register_feature_searcher(face_service_implemention::nsg_algorithm, [](int dimension, std::string path) { return std::make_shared<nsg_search_impl>(dimension, path); }), int{});
	}
}

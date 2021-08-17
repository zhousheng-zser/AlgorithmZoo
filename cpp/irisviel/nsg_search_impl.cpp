#include "nsg_search_impl.hpp"
#include "irisviel_search.hpp"
#include "feature_searcher_factory.hpp"
#include "face_service_implemention.hpp"

#include <vector>
#include <stdexcept>
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
				nsg_search_.build_graph(*current_data_);
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

		void current_data(const std::vector<const float*>& data) noexcept
		{
			current_data_ = &data;
		}

		vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const
		{
			// This implementation does not support searching by min similarity.
			if (!top_k)
			{
				throw std::invalid_argument{ u8"The top K must be set in the NSG implementation." };
			}

			auto actual_size = top_k ? std::min<std::size_t>(current_data_->size(), *top_k) : current_data_->size();

			return nsg_search_.search_vector(query_data, actual_size);
		}
	private:
		int dimension_;
		irisviel_search nsg_search_;
		const std::vector<const float*>* current_data_;
	};

	nsg_search_impl::nsg_search_impl(int dimension) : impl_{ std::make_unique<impl>(dimension) }
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

	void nsg_search_impl::current_data(const std::vector<const float*>& data) noexcept
	{
		impl_->current_data(data);
	}

	vector2d<std::tuple<std::uint32_t, float>> nsg_search_impl::search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const
	{
		return impl_->search_vector(query_data, min_similarity, top_k);
	}

	namespace
	{
		int register_hint = (register_feature_searcher(face_service_implemention::nsg_algorithm, [](int dimension) { return std::make_shared<nsg_search_impl>(dimension); }), int{});
	}
}

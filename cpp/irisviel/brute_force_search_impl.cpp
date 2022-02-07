#include "distance.hpp"
#include "brute_force_search_impl.hpp"
#include "memory_resource_adapter.hpp"
#include "feature_searcher_factory.hpp"
#include "face_service_implemention.hpp"

#include <vector>
#include <cstring>
#include <numeric>
#include <algorithm>
#include <functional>

#include <Julius/julius_gemv.hpp>

namespace glasssix::irisviel
{
	class brute_force_search_impl::impl
	{
	public:
		impl(int dimension) : dimension_{ dimension }, current_data_{}, pool_{ make_synchronized_pool_resource_workaround() }, float_allocator_{ pool_.get() }, size_t_allocator_{ pool_.get() }
		{
		}

		int dimension() const noexcept
		{
			return dimension_;
		}

		void current_data(const std::vector<database_feature_observer::feature>& data) noexcept
		{
			current_data_ = &data;
		}

		vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k)
		{
			vector2d<std::tuple<std::uint32_t, float>> result;

			for (auto&& item : query_data)
			{
				result.emplace_back(search_single_vector(item, min_similarity, top_k));
			}

			return result;
		}
	private:
		std::vector<std::tuple<std::uint32_t, float>> search_single_vector(const float* query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k)
		{
			std::vector<std::tuple<std::uint32_t, float>> result;

			if (current_data_ == nullptr)
			{
				return result;
			}

			// Allocates intermediate buffers.
			auto total_feature_size = dimension_ * current_data_->size();
			hide_exp::pmr::vector<float> input_buffer(total_feature_size, float_allocator_);
			hide_exp::pmr::vector<float> output_buffer(current_data_->size(), float_allocator_);
			hide_exp::pmr::vector<std::size_t> output_indices(current_data_->size(), size_t_allocator_);
			auto iter = input_buffer.begin();

			// Filles in the indices with 0, 1, 2, ...
			std::iota(output_indices.begin(), output_indices.end(), 0);

			// Copies all data into the temporary buffer.
			for (auto&& item : *current_data_)
			{
				iter = std::transform(item.data, item.data + dimension_, iter, [&](float inner) { return inner / item.feature_modulo_length; });
			}

			// Calculates the scores of all features.
			excalibur::juliusblas::cblas_sgemv_AnoTrans(static_cast<int>(current_data_->size()), dimension_, 1.f, input_buffer.data(), dimension_, query_data, 1, 0.f, output_buffer.data(), 1);
			std::sort(output_indices.begin(), output_indices.end(), [&](std::size_t left, std::size_t right) { return output_buffer[left] > output_buffer[right]; });

			// Creates a handler to check the similarity condition.
			const auto check_similarity{ min_similarity ? std::function{ [&](float similarity) { return similarity > *min_similarity; } } : [](float similarity) { return true; } };

			// Calculates the cosine distances as final similarities.
			auto real_size = top_k ? std::min<std::size_t>(current_data_->size(), *top_k) : current_data_->size();
			auto normalized_query_data = distance_cosine::norm(query_data, dimension_);

			result.reserve(real_size);

			for (std::size_t i = 0; i < real_size; i++)
			{
				std::size_t index = output_indices[i];
				auto normalized_current_data = distance_cosine::norm((*current_data_)[index].data, dimension_);
				auto similarity = 1.f - distance_cosine::compare((*current_data_)[index].data, normalized_current_data, query_data, normalized_query_data, dimension_);

				if (!check_similarity(similarity))
				{
					break;
				}

				result.emplace_back(index, similarity);
			}

			return result;
		}

		int dimension_;
		std::shared_ptr<hide_exp::pmr::memory_resource> pool_;
		hide_exp::pmr::polymorphic_allocator<float> float_allocator_;
		hide_exp::pmr::polymorphic_allocator<std::size_t> size_t_allocator_;
		const std::vector<database_feature_observer::feature>* current_data_;
	};

	brute_force_search_impl::brute_force_search_impl(int dimension) : impl_{ std::make_unique<impl>(dimension) }
	{
	}

	brute_force_search_impl::~brute_force_search_impl()
	{
	}

	int brute_force_search_impl::dimension() const noexcept
	{
		return impl_->dimension();
	}

	void brute_force_search_impl::build_cache() const
	{
	}

	void brute_force_search_impl::save_cache(std::string_view path) const
	{
	}

	void brute_force_search_impl::load_cache(std::string_view path) const
	{
	}

	void brute_force_search_impl::current_data(const std::vector<database_feature_observer::feature>& data) noexcept
	{
		impl_->current_data(data);
	}

	vector2d<std::tuple<std::uint32_t, float>> brute_force_search_impl::search_vector(const std::vector<const float*>& query_data, std::optional<float> min_similarity, std::optional<std::uint32_t> top_k) const
	{
		return impl_->search_vector(query_data, min_similarity, top_k);
	}

	namespace
	{
		int register_hint = (register_feature_searcher(face_service_implemention::brute_force, [](int dimension) { return std::make_shared<brute_force_search_impl>(dimension); }), int{});
	}
}

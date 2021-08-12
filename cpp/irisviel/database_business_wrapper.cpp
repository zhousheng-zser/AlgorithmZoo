#include "database_business_wrapper.hpp"
#include "filesystem_utils.hpp"
#include "irisviel_search.hpp"
#include "feature_searcher.hpp"
#include "nsg_calculate_error.hpp"
#include "brute_force_search_impl.hpp"
#include "feature_searcher_factory.hpp"

#include <cstddef>
#include <utility>
#include <fstream>
#include <algorithm>

namespace glasssix
{
	namespace irisviel
	{
		namespace
		{
			const fs::path cache_extension{ ".idx" };
		}

		class database_business_wrapper::impl
		{
		public:
			impl(face_service_implemention implementation, const std::shared_ptr<database_feature_observer>& observer, const fs::path& map_file_path, const fs::path& cache_directory) : valid_state_{}, mark_for_deletion_{}, map_file_path_{ map_file_path }, cache_file_path_{ cache_directory / map_file_path_.filename().replace_extension(cache_extension) }, cache_directory_{ cache_directory }, implementation_{ implementation }, observer_{ observer }
			{
			}

			~impl()
			{
				if (mark_for_deletion_)
				{
					utils::safe_remove_file(cache_file_path_);
				}
			}

			std::string cache_file_path() const
			{
				return cache_file_path_.string();
			}

			void mark_for_deletion() noexcept
			{
				mark_for_deletion_ = true;
			}

			bool build(bool rebuild)
			{
				current_data_ = (*observer_)();

				if (current_data_.size() < 1)
				{
					return false;
				}

				auto safe_handler = [&](auto&& handler)
				{
					valid_state_ = false;

					try
					{
						std::forward<decltype(handler)>(handler)();
						valid_state_ = true;

						return valid_state_;
					}
					catch (nsg_calculate_error&)
					{
						return false;
					}
					catch (std::bad_alloc&)
					{
						return false;
					}
				};

				auto dimension = observer_->dimension();

				// Build the data.
				// We must catch the exceptions of infinite numbers here.
				if (!safe_handler([&] { searcher_ = make_shared_feature_searcher(implementation_, dimension); }))
				{
					return false;
				}

				searcher_->current_data(current_data_);

				if (current_data_.size() > 1)
				{
					auto cache_file_path = cache_file_path_.string();

					// rebuild only if the file does not exist.
					if (rebuild || !std::ifstream{ cache_file_path, std::ios::binary }.is_open())
					{
						if (!safe_handler([&] { searcher_->build_cache(); }))
						{
							return false;
						}

						searcher_->save_cache(cache_file_path.c_str());
					}

					// Load the searcher.
					try
					{
						searcher_->load_cache(cache_file_path.c_str());
					}
					// Note: we check the file very strictly to ensure that it is a valid file.
					// So there should almost be an exception if the file was created when the power shut down unexpectedly.
					catch (nsg_calculate_error&)
					{
						if (!safe_handler([&] { searcher_->build_cache(); }))
						{
							return false;
						}

						searcher_->save_cache(cache_file_path.c_str());
						searcher_->load_cache(cache_file_path.c_str());
					}
				}

				return true;
			}

			std::vector<database_search_result> search(const float* feature, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const
			{
				return search_many({ feature }, min_similarity, top).front();
			}

			std::vector<std::vector<database_search_result>> search_many(const std::vector<const float*>& features, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const
			{
				int dimension = observer_->dimension();
				std::vector<std::vector<database_search_result>> result;

				// We only search the result when the current state is valid.
				if (!valid_state_)
				{
					return result;
				}

				try
				{
					if (current_data_.size() < 1 || !searcher_)
					{
						return result;
					}

					auto raw_search_result = searcher_->search_vector(features, min_similarity, top);

					for (auto&& item : raw_search_result)
					{
						std::vector<database_search_result> inner;

						for (auto [index, similarity] : item)
						{
							if (current_data_.size() == 1)
							{
								index = std::min(index, 0U);
							}

							if (index >= current_data_.size())
							{
								continue;
							}

							// Retrieve the orginal data in the mapping file.
							auto offset = reinterpret_cast<const std::uint8_t*>(current_data_[index]) - database_record::feature_offset(dimension);
							auto result = database_record::create(dimension, const_cast<std::uint8_t*>(offset));
							auto orginal_data = reinterpret_cast<const database_record*>(reinterpret_cast<const std::uint8_t*>(current_data_[index]) - database_record::feature_offset(dimension));

							inner.emplace_back(database_search_result{ result, similarity });
						}

						result.emplace_back(inner);
					}

					return result;
				}
				catch (...)
				{
				}

				return result;
			}
		private:
			bool valid_state_;
			bool mark_for_deletion_;
			fs::path map_file_path_;
			fs::path cache_file_path_;
			fs::path cache_directory_;
			std::vector<const float*> current_data_;
			face_service_implemention implementation_;
			std::shared_ptr<feature_searcher> searcher_;
			std::shared_ptr<database_feature_observer> observer_;
		};

		database_business_wrapper::database_business_wrapper(face_service_implemention implementation, const std::shared_ptr<database_feature_observer>& observer, std::string_view map_file_path, std::string_view cache_directory) : impl_{ std::make_unique<impl>(implementation, observer, utils::path_from_string_view(map_file_path), utils::path_from_string_view(cache_directory)) }
		{
		}

		database_business_wrapper::~database_business_wrapper()
		{
		}

		bool database_business_wrapper::build(bool rebuild)
		{
			return impl_->build(rebuild);
		}

		void database_business_wrapper::mark_for_deletion() noexcept
		{
			impl_->mark_for_deletion();
		}

		std::string database_business_wrapper::cache_file_path() const
		{
			return impl_->cache_file_path();
		}

		std::vector<database_search_result> database_business_wrapper::search(const float* feature, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const
		{
			return impl_->search(feature, min_similarity, top);
		}

		std::vector<std::vector<database_search_result>> database_business_wrapper::search_many(const std::vector<const float*>& features, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const
		{
			return impl_->search_many(features, min_similarity, top);
		}
	}
}

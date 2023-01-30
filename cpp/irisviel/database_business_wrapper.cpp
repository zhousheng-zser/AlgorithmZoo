#include "database_business_wrapper.hpp"
#include "filesystem_utils.hpp"
#include "irisviel_search.hpp"
#include "feature_searcher.hpp"
#include "nsg_calculate_error.hpp"
#include "brute_force_search_impl.hpp"
#include "feature_searcher_factory.hpp"

#include "fmt/format.h"

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
			impl(face_service_implemention implementation, const std::shared_ptr<database_feature_observer>& observer, const fs::path& map_file_path, const fs::path& cache_directory, const fs::path& lsh_directory) : valid_state_{}, mark_for_deletion_{}, map_file_path_{ map_file_path }, cache_file_path_{ cache_directory / map_file_path_.filename().replace_extension(cache_extension) }, cache_directory_{ cache_directory }, implementation_{ implementation }, observer_{ observer }, lsh_directory_{ lsh_directory }
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
				auto safe_handler = [&](auto&& handler)
				{
					valid_state_ = false;

					try
					{
						std::forward<decltype(handler)>(handler)();
						valid_state_ = true;

						return valid_state_;
					}
					catch (const nsg_calculate_error&)
					{
						return false;
					}
					catch (const std::bad_alloc&)
					{
						return false;
					}
				};

				if (implementation_ == face_service_implemention::lsh_algorithm)
				{
					if (!safe_handler([&] { searcher_ = make_shared_feature_searcher(implementation_, observer_->dimension(), lsh_directory_.string()); }))
					{
						return false;
					}
					return true;
				}

				if (current_data_.size() < 1)
				{
					return false;
				}

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

			bool add(database_record &record) 
			{
				if (!searcher_)
				{
					build(true);
					if (!searcher_)
					{
						printf("Null searcher.\n");
						throw std::invalid_argument{ "Null searcher." };
					}
				}
				return searcher_->add(record);
			}

			std::vector<bool> remove(std::vector<std::string>& keys)
			{
                if (!searcher_)
                {
                    throw std::invalid_argument{ "Null searcher." };
                }

				return searcher_->remove(keys);
			}
			
			std::vector<bool> update(const std::vector<std::shared_ptr<database_record>>& records) const
			{
                if (!searcher_)
                {
                    throw std::invalid_argument{ "Null searcher." };
                }

				return searcher_->update(records);
			}


			std::vector<database_search_result> search(const float* feature, std::optional<float> min_similarity, std::optional<std::uint32_t> top, bool result_has_feature) const
			{
				auto result = search_many({ feature }, min_similarity, top, result_has_feature);

				return result.empty() ? std::vector<database_search_result>{} : result.front();
			}

			std::vector<std::vector<database_search_result>> search_many(const std::vector<const float*>& features, std::optional<float> min_similarity, std::optional<std::uint32_t> top, bool result_has_feature) const try
			{
				auto dimension = observer_->dimension();

				// We only search the result when the current state is valid.
				if (!valid_state_)
				{
					return {};
				}

				if (face_service_implemention::lsh_algorithm != implementation_ &&( current_data_.size() < 1 || !searcher_ ))
				{
					return {};
				}
				
				return searcher_->search_vector(features, min_similarity, top, result_has_feature);
			}
			catch (const std::exception& ex)
			{
				throw std::runtime_error{ fmt::format("Searching operation failed: {}", ex.what()) };
			}
			
			std::uint64_t count() const
			{
				if (!searcher_)
				{
					throw std::invalid_argument{ "Null searcher." };
				}

				return searcher_->count();

			}
			bool contains(std::string_view key) const
			{
				if (!searcher_)
				{
					throw std::invalid_argument{ "Null searcher." };
				}

				return searcher_->contains(key);

			}

		private:
			bool valid_state_;
			bool mark_for_deletion_;
			fs::path map_file_path_;
			fs::path cache_file_path_;
			fs::path cache_directory_;
			fs::path lsh_directory_;
			face_service_implemention implementation_;
			std::shared_ptr<feature_searcher> searcher_;
			std::shared_ptr<database_feature_observer> observer_;
			std::vector<database_feature_observer::feature> current_data_;
		};

		database_business_wrapper::database_business_wrapper(face_service_implemention implementation, const std::shared_ptr<database_feature_observer>& observer, std::string_view map_file_path, std::string_view cache_directory, std::string_view lsh_directory) : impl_{ std::make_unique<impl>(implementation, observer, utils::path_from_string_view(map_file_path), utils::path_from_string_view(cache_directory),utils::path_from_string_view(lsh_directory)) }
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

		std::vector<database_search_result> database_business_wrapper::search(const float* feature, std::optional<float> min_similarity, std::optional<std::uint32_t> top, bool result_has_feature) const
		{
			return impl_->search(feature, min_similarity, top, result_has_feature);
		}

		std::vector<std::vector<database_search_result>> database_business_wrapper::search_many(const std::vector<const float*>& features, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const
		{
			return impl_->search_many(features, min_similarity, top, true);
		}

		bool database_business_wrapper::add(database_record& record)
		{
			return impl_->add(record);
		}

		std::vector<bool> database_business_wrapper::remove(std::vector<std::string>& keys)
		{
			return impl_->remove(keys);
		}

		std::vector<bool> database_business_wrapper::update(const std::vector<std::shared_ptr<database_record>>& records) const
		{
			return impl_->update(records);
		}
		
		std::uint64_t database_business_wrapper::count() const
		{
			return impl_->count();

		}
		bool database_business_wrapper::contains(std::string_view key) const
		{
			return impl_->contains(key);

		}
	}
}

#include "face_service_internal.hpp"
#include "database_cache.hpp"
#include "filesystem_utils.hpp"
#include "Primitives/fmt/format.h"

#include <list>
#include <mutex>
#include <limits>
#include <fstream>
#include <cstddef>
#include <numeric>
#include <utility>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace glasssix
{
    namespace irisviel
    {
        namespace
        {
            const fs::path cache_folder{ "tmp" };
            const fs::path database_folder{ "data" };
            const fs::path database_extension{ ".map" };
            const fs::path lsh_folder{ "LSH_bucket" };
        }

        class face_service_internal::impl
        {
        public:
            impl(face_service_implemention implementation, int single_database_capacity, int dimension, const fs::path& working_directory) : dimension_{ dimension }, single_database_capacity_{ single_database_capacity }, database_directory_{ working_directory / database_folder }, cache_directory_{ working_directory / cache_folder }, lsh_directory_{ working_directory / lsh_folder }, implementation_{ implementation }
            {
                utils::safe_create_directories(database_directory_);
                utils::safe_create_directories(cache_directory_);
                utils::safe_create_directories(lsh_directory_);
            }

            int dimension() const noexcept
            {
                return dimension_;
            }

            std::string database_diectory() const
            {
                return database_directory_.string();
            }

            std::string cache_directory() const
            {
                return cache_directory_.string();
            }

            std::string lsh_directory() const
            {
                return lsh_directory_.string();
            }

            void load_databases()
            {
                std::error_code code;
                std::scoped_lock guard{ lock_ };

                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    std::ofstream fp(lsh_directory() + "\\data.map", std::fstream::out | std::ios::app);
                    fp.close();  //Generate empty files for compatibility with other algorithms
                    auto cache = create_new_database_core(lsh_directory() + "\\data.map");

                    cache->wrapper->build(false);
                }
                else
                {
                    for (auto& item : fs::directory_iterator{ database_directory_, fs::directory_options::skip_permission_denied, code })
                    {
                        if (item.path().filename().extension() == database_extension)
                        {
                            auto cache = create_new_database_core(item.path().string());

                            // Builds the existing database.
                            cache->wrapper->build(false);
                        }
                    }
                }
            }

            std::uint64_t record_count() const
            {
                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    auto& item = *cache_.begin();
                    return item->wrapper->count();
                }
                else
                {
                    return std::accumulate(
                        cache_.begin(),
                        cache_.end(),
                        0ULL,
                        [&](std::uint64_t init, const std::shared_ptr<database_cache>& item) { return init + item->manager->count(); });

                }
            }

            bool contains_key(std::string_view key) const
            {
                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    auto& item = *cache_.begin();
                    return item->wrapper->contains(key);
                }
                else
                {
                    return std::any_of(cache_.begin(), cache_.end(), [&](const std::shared_ptr<database_cache>& inner) { return inner->manager->contains(key); });
                }
            }

            std::shared_ptr<database_record> try_get_record(std::string_view key) const
            {
                for (auto&& item : cache_)
                {
                    if (auto record = item->manager->try_get_record(key))
                    {
                        return record;
                    }
                }

                return nullptr;
            }

            void clear() noexcept
            {
                std::scoped_lock guard{ lock_ };

                cache_.clear();
            }

            void remove_all()
            {
                std::scoped_lock guard{ lock_ };

                std::for_each(cache_.begin(), cache_.end(), [](const std::shared_ptr<database_cache> item) { item->mark_for_deletion(); });
                cache_.clear();

                // Removes all remaining contents.
                utils::safe_remove_directories(cache_directory_);
                utils::safe_remove_directories(database_directory_);
                utils::safe_remove_directories(lsh_directory_);
            }

            std::vector<database_search_result> search(const float* feature, std::uint32_t top)
            {
                return search_internal(feature, std::nullopt, top);
            }

            std::vector<database_search_result> search(const float* feature, float min_similarity, std::optional<std::uint32_t> top)
            {
                return search_internal(feature, min_similarity, top);
            }

            std::vector<bool> add(const std::vector<std::shared_ptr<database_record>>& records)
            {
                std::scoped_lock guard{ lock_ };

                std::size_t index{};
                std::vector<bool> result(records.size());
                std::unordered_set<std::shared_ptr<database_cache>> changed_databases;

                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    for (auto& record : records)
                    {
                        auto& item = *cache_.begin();

                        result[index++] = item->wrapper->add(*record);
                    }
                }
                else
                {
                    for (auto& record : records)
                    {
                        auto item = find_available_database_core(record->key());
                        std::string _key(record->key());
                        auto success = item && item->manager->add(*record);

                        result[index++] = success;

                        if (success)
                        {
                            changed_databases.emplace(std::move(item));
                        }
                    }

                    // Builds the changed databases.
                    for (auto& item : changed_databases)
                    {
                        item->commit();
                    }
                }

                return result;
            }

            std::vector<bool> remove(std::vector<std::string>& keys)
            {
                std::scoped_lock guard{ lock_ };

                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    auto& item = *cache_.begin();

                    return item->wrapper->remove(keys);
                }
                else
                {
                    std::size_t index{};
                    std::vector<bool> result(keys.size());

                    auto item_predicate = [&](database_cache& item, const std::string& key)
                    {
                        auto success = item.manager->remove(key);

                        result[index++] = success;

                        return success && !item.manager->empty();
                    };

                    remove_if_core([&](database_cache& item) { return std::count_if(keys.begin(), keys.end(), std::bind(item_predicate, item, std::placeholders::_1)) > 0; });

                    return result;
                }
            }

            std::vector<bool> update_more(const std::vector<std::shared_ptr<database_record>>& records)
            {
                std::scoped_lock guard{ lock_ };

                if (implementation_ == face_service_implemention::lsh_algorithm)
                {
                    auto& item = *cache_.begin();
                    
                    return item->wrapper->update(records);
                }
                else
                {
                    std::size_t index{};
                    std::vector<bool> result(records.size());

                    for (auto& item : cache_)
                    {
                        auto item_predicate = [&](const std::shared_ptr<database_record>& record)
                        {
                            auto success = item->manager->update(*record);

                            result[index++] = success;

                            return success;
                        };

                        std::ptrdiff_t count = std::count_if(records.begin(), records.end(), item_predicate);

                        if (count > 0)
                        {
                            item->commit();
                        }
                    }

                    return result;
                }
            }
        private:
            std::vector<database_search_result> search_internal(const float* feature, std::optional<float> similarity, std::optional<std::uint32_t> top)
            {
                std::scoped_lock guard{ lock_ };
                std::vector<database_search_result> result;


                for (auto& item : cache_)
                {
                    auto search_result = item->wrapper->search(feature, similarity, top);

                    if (!search_result.empty())
                    {
                        std::copy(search_result.begin(), search_result.end(), std::back_inserter(result));
                    }
                }
                if (face_service_implemention::lsh_algorithm == implementation_)
                    return result;

                std::sort(result.begin(), result.end(), [](const database_search_result& left, const database_search_result& right) { return left.similarity > right.similarity; });

                if (top)
                {
                    result.resize(std::min<std::size_t>(*top, result.size()));
                }

                return result;
            }

            template<typename Predicate>
            void remove_if_core(Predicate&& predicate)
            {
                for (auto iter = cache_.begin(); iter != cache_.end(); )
                {
                    if (std::forward<Predicate>(predicate)(**iter))
                    {
                        (*iter)->commit();
                    }

                    // Deletes the database if it is empty.
                    if ((*iter)->manager->empty())
                    {
                        iter = cache_.erase(iter);
                    }
                    else
                    {
                        iter++;
                    }
                }
            }

            std::shared_ptr<database_cache> find_available_database_core(std::string_view key)
            {
                // Finds a database that can accommodate at least one record and ensures there is no repeated key among the databases.
                for (auto& item : cache_)
                {
                    // Ensures the uniqueness of the key.
                    if (item->manager->contains(key))
                    {
                        return nullptr;
                    }

                    if (!item->manager->full())
                    {
                        return item;
                    }
                }

                utils::safe_create_directories(cache_directory_);
                utils::safe_create_directories(database_directory_);
                utils::safe_create_directories(lsh_directory_);

                // Generates a new empty database.
                auto uuid = boost::uuids::to_string(boost::uuids::random_generator{}());
                auto file_path = database_directory_ / fmt::format("{}{}", uuid, database_extension.string());
                std::ofstream{ file_path, std::ios::trunc | std::ios::binary };

                return create_new_database_core(file_path.string());
            }

            std::shared_ptr<database_cache> create_new_database_core(std::string_view path)
            {
                auto manager = std::make_shared<database_manager>(path, single_database_capacity_, dimension_);
                auto wrapper = std::make_shared<database_business_wrapper>(implementation_, manager->create_feature_observer(), path, cache_directory_.string(), lsh_directory_.string());

                return cache_.emplace_back(std::make_shared<database_cache>(std::move(manager), std::move(wrapper)));
            }

            int dimension_;
            int single_database_capacity_;

            std::mutex lock_;
            fs::path cache_directory_;
            fs::path database_directory_;
            fs::path lsh_directory_;
            face_service_implemention implementation_;
            std::list<std::shared_ptr<database_cache>> cache_;
        };

        face_service_internal::face_service_internal(face_service_implemention implementation, int single_database_capacity, int dimension, std::string_view working_directory) : impl_{ std::make_unique<impl>(implementation, single_database_capacity, dimension, utils::path_from_string_view(working_directory)) }
        {
        }

        face_service_internal::~face_service_internal()
        {
        }

        void face_service_internal::clear() noexcept
        {
            impl_->clear();
        }

        void face_service_internal::remove_all() noexcept
        {
            impl_->remove_all();
        }

        int face_service_internal::dimension() const noexcept
        {
            return impl_->dimension();
        }

        std::string face_service_internal::database_directory() const
        {
            return impl_->database_diectory();
        }

        std::string face_service_internal::cache_directory() const
        {
            return impl_->cache_directory();
        }

        std::string face_service_internal::lsh_directory() const
        {
            return impl_->lsh_directory();
        }

        void face_service_internal::load_databases()
        {
            impl_->load_databases();
        }

        std::uint64_t face_service_internal::record_count() const
        {
            return impl_->record_count();
        }

        bool face_service_internal::contains_key(std::string_view key) const
        {
            return impl_->contains_key(key);
        }

        std::shared_ptr<database_record> face_service_internal::try_get_record(std::string_view key) const
        {
            return impl_->try_get_record(key);
        }

        std::vector<database_search_result> face_service_internal::search(const float* feature, std::uint32_t top) const
        {
            return impl_->search(feature, top);
        }

        std::vector<database_search_result> face_service_internal::search(const float* feature, float min_similarity, std::optional<std::uint32_t> top) const
        {
            return impl_->search(feature, min_similarity, top);
        }

        std::vector<bool> face_service_internal::add(const std::vector<std::shared_ptr<database_record>>& records)
        {
            return impl_->add(records);
        }

        std::vector<bool> face_service_internal::remove(std::vector<std::string>& keys)
        {
            return impl_->remove(keys);
        }

        std::vector<bool> face_service_internal::update(const std::vector<std::shared_ptr<database_record>>& records)
        {
            return impl_->update_more(records);
        }
    }
}

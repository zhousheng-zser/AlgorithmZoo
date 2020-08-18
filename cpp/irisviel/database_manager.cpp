#include "database_manager.hpp"
#include "database_header.hpp"
#include "memory_mapping_operator.hpp"
#include "Primitives/hash_utils.hpp"

#include <utility>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace glasssix
{
	namespace irisviel
	{
		namespace
		{
			constexpr int database_header_version = 1000;
		}

		class database_manager::impl
		{
		public:
			impl(std::string_view file_path, std::size_t capacity, int dimension) : dimension_{ dimension }, removal_pending_{}, record_size_{ database_record::record_size(dimension) }, mapping_{ file_path, (capacity + 1UL) * database_record::record_size(dimension) }
			{
				auto header = mapping_.locate_element_absolutely<database_header>(0);

				if (!header)
				{
					throw std::runtime_error{ "Some error occurs when reading the data header." };
				}

				header_ = *header;

				if (header_.capacity <= 0)
				{
					header_.capacity = static_cast<int>(capacity);
					header_.current_position = 1;
					header_.version = database_header_version;
					mapping_.write_element_absolutely(0, header_);
				}

				// Builds the index for the key.
				build_index_core();
			}

			~impl()
			{
				save_changes();
			}

			bool add(database_record& data)
			{
				if (full() || contains(data.key()))
				{
					return false;
				}

				// Appends an new item at the end.
				data.active(true);
				mapping_.write_dynamic_buffer(header_.current_position, data);
				record_entries_.emplace(data.key(), header_.current_position);

				// Writes the new position.
				update_current_position_core([](int& position) { position++; });

				return true;
			}

			bool contains(std::string_view key)
			{
				return record_entries_.find(std::string{ key }) != record_entries_.end();
			}

			bool update(database_record& data)
			{
				auto iter = record_entries_.find(std::string(data.key()));

				if (iter == record_entries_.end())
				{
					return false;
				}

				mapping_.write_dynamic_buffer(iter->second, data);

				return true;
			}

			bool remove(std::string_view key)
			{
				if (record_entries_.erase(std::string{ key }) > 0)
				{
					removal_pending_ = true;

					return true;
				}

				return false;
			}

			bool empty() const noexcept
			{
				return header_.current_position <= 1 && record_entries_.empty();
			}

			bool full() const noexcept
			{
				return header_.current_position > header_.capacity || record_entries_.size() >= header_.capacity;
			}

			void mark_for_deletion() noexcept
			{
				mapping_.mark_for_deletion();
			}

			std::shared_ptr<database_feature_observer> create_feature_observer()
			{
				return std::make_shared<database_feature_observer>([this]
					{
						std::vector<const float*> result;

						for (auto [key, index] : record_entries_)
						{
							auto entry = mapping_.locate_element_bytes(index, record_size_);
							auto data_ref = database_record::create_ref(dimension_, entry);

							result.emplace_back(data_ref->feature().data());
						}

						return result;
					}, dimension_);
			}

			void save_changes() noexcept
			{
				// Synchronizes the data if any removal operation is pending.
				if (removal_pending_)
				{
					int new_index = 1;

					for (int i = 1; i < header_.current_position && new_index <= record_entries_.size(); i++)
					{
						auto entry = mapping_.locate_element_bytes(i, record_size_);
						auto data_ref = database_record::create_ref(dimension_, entry);

						if (data_ref && data_ref->active() && record_entries_.find(std::string(data_ref->key())) != record_entries_.end())
						{
							mapping_.write_element_bytes(new_index++, entry, record_size_);
						}
					}

					update_current_position_core([&](int& position) { position = new_index; });
					removal_pending_ = false;
				}
				
				mapping_.save_changes();
			}

			std::string file_path() const
			{
				return mapping_.path();
			}
		private:
			template<typename Handler>
			void update_current_position_core(Handler&& handler)
			{
				std::forward<Handler>(handler)(header_.current_position);
				mapping_.write_element_absolutely(database_header_traits::current_position_offset, header_.current_position);
			}

			void build_index_core()
			{
				record_entries_.clear();

				for (int i = 1; i < header_.current_position; i++)
				{
					auto entry = mapping_.locate_element_bytes(i, record_size_);
					auto data_ref = database_record::create_ref(dimension_, entry);

					if (data_ref && data_ref->active())
					{
						record_entries_.emplace(data_ref->key(), i);
					}
				}
			}

			int dimension_;
			bool removal_pending_;
			database_header header_;
			std::size_t record_size_;
			memory_mapping_operator mapping_;
			std::unordered_map<std::string, int, case_insensitive_string_hash, case_insensitive_string_comparer> record_entries_;
		};

		database_manager::database_manager(std::string_view file_path, std::size_t capacity, int dimension) : impl_{ new impl{ file_path, capacity, dimension } }
		{
		}

		database_manager::~database_manager()
		{
			if (impl_)
			{
				delete impl_;
				impl_ = nullptr;
			}
		}

		bool database_manager::contains(std::string_view key)
		{
			return impl_->contains(key);
		}

		bool database_manager::update(database_record& record)
		{
			return impl_->update(record);
		}

		bool database_manager::remove(std::string_view key)
		{
			return impl_->remove(key);
		}

		bool database_manager::empty() const noexcept
		{
			return impl_->empty();
		}

		bool database_manager::full() const noexcept
		{
			return impl_->full();
		}

		bool database_manager::add(database_record& record)
		{
			return impl_->add(record);
		}

		void database_manager::mark_for_deletion() noexcept
		{
			impl_->mark_for_deletion();
		}

		std::shared_ptr<database_feature_observer> database_manager::create_feature_observer()
		{
			return impl_->create_feature_observer();
		}

		void database_manager::save_changes() noexcept
		{
			impl_->save_changes();
		}

		std::string database_manager::file_path() const
		{
			return impl_->file_path();
		}
	}
}

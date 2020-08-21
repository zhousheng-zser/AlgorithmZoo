#pragma once

#include "database_record.hpp"
#include "database_feature_observer.hpp"

#include <memory>
#include <vector>
#include <string>
#include <cstddef>
#include <string_view>

namespace glasssix
{
	namespace irisviel
	{
		class database_manager
		{
		public:
			class impl;

			database_manager(std::string_view file_path, std::size_t capacity, int dimension);
			virtual ~database_manager();
			bool contains(std::string_view key);
			bool update(database_record& record);
			bool remove(std::string_view key);
			bool empty() const noexcept;
			bool full() const noexcept;
			bool add(database_record& record);
			void mark_for_deletion() noexcept;
			std::shared_ptr<database_feature_observer> create_feature_observer();
			void save_changes() noexcept;
			std::string file_path() const;
		private:
			impl* impl_;
		};
	}
}

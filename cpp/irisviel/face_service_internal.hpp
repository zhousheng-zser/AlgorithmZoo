#pragma once

#include "database_record.hpp"
#include "database_search_result.hpp"
#include "face_service_implemention.hpp"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <string_view>

namespace glasssix
{
	namespace irisviel
	{
		class face_service_internal
		{
		public:
			class impl;

			face_service_internal(face_service_implemention implementation, int single_database_capacity, int dimension, std::string_view working_directory);
			virtual ~face_service_internal();
			void clear() noexcept;
			void remove_all() noexcept;
			int dimension() const noexcept;
			std::string database_directory() const;
			std::string cache_directory() const;
			std::uint64_t record_count() const;
			bool contains_key(std::string_view key) const;
			void load_databases();
			std::vector<database_search_result> search(const float* feature, std::uint32_t top) const;
			std::vector<database_search_result> search(const float* feature, float min_similarity, std::optional<std::uint32_t> top) const;
			void add(database_record& record);
			void add(const std::vector<std::shared_ptr<database_record>>& records);
			void remove(std::string_view key);
			void remove(const std::vector<std::string>& keys);
			void update(database_record& record);
			void update(const std::vector<std::shared_ptr<database_record>>& records);
		private:
			std::unique_ptr<impl> impl_;
		};
	}
}

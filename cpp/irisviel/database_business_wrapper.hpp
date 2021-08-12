#pragma once

#include "database_search_result.hpp"
#include "database_feature_observer.hpp"
#include "face_service_implemention.hpp"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>
#include <string_view>

namespace glasssix
{
	namespace irisviel
	{
		class database_business_wrapper
		{
		public:
			class impl;

			database_business_wrapper(face_service_implemention implementation, const std::shared_ptr<database_feature_observer>& observer, std::string_view map_file_path, std::string_view cache_directory);
			virtual ~database_business_wrapper();
			bool build(bool rebuild);
			void mark_for_deletion() noexcept;
			std::string cache_file_path() const;
			std::vector<database_search_result> search(const float* feature, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const;
			std::vector<std::vector<database_search_result>> search_many(const std::vector<const float*>& features, std::optional<float> min_similarity, std::optional<std::uint32_t> top) const;
		private:
			std::unique_ptr<impl> impl_;
		};
	}
}

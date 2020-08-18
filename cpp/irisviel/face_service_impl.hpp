#pragma once

#include "face_service.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	inline constexpr exposing::utf8_string_view irisviel_face_service_qualified_name{ u8"g6.irisviel.faceService" };

	class face_service_internal;

	class face_service_impl : public exposing::implements<face_service_impl, face_service>, public exposing::make_external_qualified_name<irisviel_face_service_qualified_name>
	{
	public:
		void init(std::int32_t single_database_capacity, std::int32_t dimension, exposing::utf8_string_view working_directory);
		void clear() const;
		void remove_all() const;
		std::int32_t dimension() const;
		exposing::param_string database_directory() const;
		exposing::param_string cache_directory() const;
		void load_databases() const;
		void add_record(const record& record) const;
		void add_records(const exposing::param_vector<record>& records) const;
		void remove_record(const exposing::param_string& key) const;
		void remove_records(const exposing::param_vector<exposing::param_string>& keys) const;
		void update_record(const irisviel::record& record) const;
		void update_records(const exposing::param_vector<record>& records) const;
		exposing::param_vector<irisviel::search_result> search(const exposing::param_vector<float>& feature, std::int32_t top_count_to_retrieve) const;
		exposing::param_vector<irisviel::search_result> search(exposing::param_span<const float> feature, std::int32_t top_count_to_retrieve) const;
	private:
		std::shared_ptr<face_service_internal> impl_;
	};
}

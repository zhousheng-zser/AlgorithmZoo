#pragma once

#include "face_service.hpp"

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	class face_service_internal;

	class face_service_impl : public exposing::implements<face_service_impl, face_service>
	{
	public:
		face_service_impl(std::int32_t single_database_capacity, std::int32_t dimension, exposing::utf8_string_view working_directory);
		virtual ~face_service_impl();
		void clear() const;
		void remove_all() const;
		exposing::param_string database_directory() const;
		exposing::param_string cache_directory() const;
		void load_databases() const;
		void add_record(const record& record) const;
		void add_records(const exposing::param_vector<record>& records) const;
		void remove_record(const exposing::param_string& key) const;
		void remove_records(const exposing::param_vector<exposing::param_string>& keys) const;
		void update_record(const irisviel::record& record) const;
		void update_records(const exposing::param_vector<record>& records) const;
	private:
		face_service_internal* impl_;
	};
}

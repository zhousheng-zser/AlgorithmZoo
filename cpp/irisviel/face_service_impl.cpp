#include "face_service_impl.hpp"
#include "face_service_internal.hpp"

namespace glasssix::irisviel
{
	namespace
	{
		std::shared_ptr<database_record> create_internal_record(const record& record)
		{
			auto feature = record.feature();
			auto internal_record = database_record::create(record.dimension());
			std::vector<float> internal_feature(exposing::begin(feature), exposing::end(feature));

			internal_record->key(exposing::to_narrow_string(record.key()).c_str());
			internal_record->feature(internal_feature);

			return internal_record;
		}
	}

	face_service_impl::face_service_impl(std::int32_t single_database_capacity, std::int32_t dimension, exposing::utf8_string_view working_directory) : impl_{ new face_service_internal{ single_database_capacity, dimension, exposing::to_narrow_string(working_directory) } }
	{
	}

	face_service_impl::~face_service_impl()
	{
		if (impl_)
		{
			delete impl_;
		}
	}

	void face_service_impl::clear() const
	{
		impl_->clear();
	}

	void face_service_impl::remove_all() const
	{
		impl_->remove_all();
	}

	exposing::param_string face_service_impl::database_directory() const
	{
		return exposing::to_param_string(impl_->database_directory());
	}

	exposing::param_string face_service_impl::cache_directory() const
	{
		return exposing::to_param_string(impl_->cache_directory());
	}

	void face_service_impl::load_databases() const
	{
		impl_->load_databases();
	}

	void face_service_impl::add_record(const record& record) const
	{
		impl_->add(*create_internal_record(record));
	}

	void face_service_impl::add_records(const exposing::param_vector<record>& records) const
	{
		std::vector<std::shared_ptr<database_record>> internal_records;

		for (const auto& item : records)
		{
			internal_records.emplace_back(create_internal_record(item));
		}
		
		impl_->add(internal_records);
	}

	void face_service_impl::remove_record(const exposing::param_string& key) const
	{
		impl_->remove(exposing::to_narrow_string(key));
	}

	void face_service_impl::remove_records(const exposing::param_vector<exposing::param_string>& keys) const
	{
		std::vector<std::string> internal_keys;

		for (const auto& item : keys)
		{
			internal_keys.emplace_back(exposing::to_narrow_string(item));
		}

		impl_->remove(internal_keys);
	}

	void face_service_impl::update_record(const irisviel::record& record) const
	{
		impl_->update(*create_internal_record(record));
	}

	void face_service_impl::update_records(const exposing::param_vector<record>& records) const
	{
		std::vector<std::shared_ptr<database_record>> internal_records;

		for (const auto& item : records)
		{
			internal_records.emplace_back(create_internal_record(item));
		}

		impl_->update(internal_records);
	}
}

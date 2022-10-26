#include "face_service_impl.hpp"
#include "face_service_internal.hpp"

#include "record_impl.hpp"
#include "search_result_impl.hpp"

#include <vector>
#include <utility>

namespace glasssix::irisviel
{
	namespace
	{
		record create_record(const database_record& internal_record)
		{
			auto record = exposing::make_as_first<record_impl>();
			
			record.init(internal_record.dimension());
			record.key(exposing::to_param_string(internal_record.key()));
			record.feature(internal_record.feature());

			return record;
		}

		std::shared_ptr<database_record> create_internal_record(const record& record)
		{
			auto feature = record.feature();
			auto internal_record = database_record::create(record.dimension());
			std::vector<float> internal_feature(exposing::begin(feature), exposing::end(feature));

			internal_record->key(exposing::to_narrow_string(record.key()));
			internal_record->feature(internal_feature);

			return internal_record;
		}

		exposing::param_vector<search_result> create_search_result(const std::vector<database_search_result>& internal_result)
		{
			auto result = exposing::make_param_vector<search_result>();

			for (const auto& item : internal_result)
			{
				result.push_back(exposing::make_as_first<search_result_impl>(item));
			}

			return result;
		}

		template<typename Container>
		void check_dimension(Container&& feature, int dimension)
		{
			if (std::forward<Container>(feature).size() != static_cast<std::size_t>(dimension))
			{
				throw exposing::abi_invalid_argument{ exposing::format(u8"The feature size {} is different from {}.", std::forward<Container>(feature).size(), dimension) };
			}
		}
	}

	face_service_impl::face_service_impl()
	{
	}

	face_service_impl::~face_service_impl()
	{
	}

	void face_service_impl::init(face_service_implemention implementation, std::int32_t single_database_capacity, std::int32_t dimension, exposing::utf8_string_view working_directory)
	{
		impl_ = std::make_unique<face_service_internal>(implementation, single_database_capacity, dimension, exposing::to_narrow_string(working_directory));
	}

	void face_service_impl::clear() const
	{
		impl_->clear();
	}

	void face_service_impl::remove_all() const
	{
		impl_->remove_all();
	}

	std::int32_t face_service_impl::dimension() const
	{
		return impl_->dimension();
	}

	exposing::param_string face_service_impl::database_directory() const
	{
		return exposing::to_param_string(impl_->database_directory());
	}

	exposing::param_string face_service_impl::cache_directory() const
	{
		return exposing::to_param_string(impl_->cache_directory());
	}

	exposing::param_string face_service_impl::lsh_directory() const
	{
		return exposing::to_param_string(impl_->lsh_directory());
	}

	void face_service_impl::load_databases() const
	{
		impl_->load_databases();
	}

	std::uint64_t face_service_impl::record_count() const
	{
		return impl_->record_count();
	}

	bool face_service_impl::contains_key(const exposing::param_string& key) const
	{
		return impl_->contains_key(key);
	}

	record face_service_impl::try_get_record(const exposing::param_string& key) const
	{
		if (auto inner_record = impl_->try_get_record(exposing::to_narrow_string(key)))
		{
			return create_record(*inner_record);
		}

		return nullptr;
	}

	void face_service_impl::add_record(const record& record) const
	{
		//   impl_->add(*create_internal_record(record));
		std::vector<std::shared_ptr<database_record>> internal_records;
		internal_records.emplace_back(create_internal_record(record));
		impl_->add(internal_records);
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
		std::string _key(key);
		std::vector<std::string>keys;
		keys.emplace_back(_key);
		
		//impl_->remove(exposing::to_narrow_string(key));
		impl_->remove(keys);
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

	void face_service_impl::update_record(const record& record) const
	{
		//impl_->update(*create_internal_record(record));
		
		std::vector<std::shared_ptr<database_record>> internal_records;
		internal_records.emplace_back(create_internal_record(record));
		impl_->update(internal_records);
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

	exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, std::uint32_t top_count_to_retrieve) const
	{
		check_dimension(feature, dimension());

		auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), top_count_to_retrieve);

		return create_search_result(internal_result);
	}

	exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, float min_similarity) const
	{
		check_dimension(feature, dimension());

		auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), min_similarity, std::nullopt);

		return create_search_result(internal_result);
	}

	exposing::param_vector<search_result> face_service_impl::search(const exposing::param_vector<float>& feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
	{
		check_dimension(feature, dimension());

		auto internal_result = impl_->search(std::vector<float>(exposing::begin(feature), exposing::end(feature)).data(), min_similarity, top_count_to_retrieve);

		return create_search_result(internal_result);
	}

	exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, std::uint32_t top_count_to_retrieve) const
	{

		check_dimension(feature, dimension());

		auto internal_result = impl_->search(feature.data(), top_count_to_retrieve);

		return create_search_result(internal_result);
	}

	exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, float min_similarity) const
	{
		check_dimension(feature, dimension());

		auto internal_result = impl_->search(feature.data(), min_similarity, std::nullopt);

		return create_search_result(internal_result);
	}

	exposing::param_vector<search_result> face_service_impl::search(exposing::param_span<const float> feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
	{
		check_dimension(feature, dimension());

		auto internal_result = impl_->search(feature.data(), min_similarity, top_count_to_retrieve);

		return create_search_result(internal_result);
	}
}

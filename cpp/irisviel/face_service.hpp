#pragma once

#include "record.hpp"
#include "search_result.hpp"
#include "face_service_implemention.hpp"

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct face_service;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<irisviel::face_service>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "361EF02B-31FE-4692-9258-0D22061CB4C8" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(std::int32_t/*abi_in_t<irisviel::face_service_implemention>*/ implementation, std::int32_t single_database_capacity, std::int32_t dimension, abi_in_t<param_string> working_directory) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL clear() noexcept = 0;
			virtual std::int32_t G6_ABI_CALL remove_all() noexcept = 0;
			virtual std::int32_t G6_ABI_CALL dimension(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL database_directory(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL load_databases() noexcept = 0;
			virtual std::int32_t G6_ABI_CALL cache_directory(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL record_count(abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL contains_key(abi_in_t<param_string> key, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL try_get_record(abi_in_t<param_string> key, abi_out_t<irisviel::record> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_record(abi_in_t<irisviel::record> record) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_records(abi_in_t<param_vector<irisviel::record>> records) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL remove_record(abi_in_t<param_string> key) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL remove_records(abi_in_t<param_vector<param_string>> keys) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL update_record(abi_in_t<irisviel::record> record) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL update_records(abi_in_t<param_vector<irisviel::record>> records) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, float min_similarity, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, float min_similarity, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, float min_similarity, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, float min_similarity, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::face_service> : interface_vtable_base<Derived, irisviel::face_service>
	{
		virtual std::int32_t G6_ABI_CALL init(std::int32_t/*abi_in_t<irisviel::face_service_implemention>*/ implementation, std::int32_t single_database_capacity, std::int32_t dimension, abi_in_t<param_string> working_directory)  noexcept override
		{
			return abi_safe_call([&] { this->self().init(/*create_from_abi*/static_cast<irisviel::face_service_implemention>(implementation), single_database_capacity, dimension, create_from_abi<param_string>(working_directory)); });
		}

		virtual std::int32_t G6_ABI_CALL clear() noexcept override
		{
			return abi_safe_call([&] { this->self().clear(); });
		}

		virtual std::int32_t G6_ABI_CALL remove_all() noexcept override
		{
			return abi_safe_call([&] { this->self().remove_all(); });
		}

		virtual std::int32_t G6_ABI_CALL dimension(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().dimension()); });
		}

		virtual std::int32_t G6_ABI_CALL database_directory(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().database_directory()); });
		}

		virtual std::int32_t G6_ABI_CALL cache_directory(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().cache_directory()); });
		}

		virtual std::int32_t G6_ABI_CALL load_databases() noexcept override
		{
			return abi_safe_call([&] { this->self().load_databases(); });
		}

		virtual std::int32_t G6_ABI_CALL record_count(abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().record_count()); });
		}

		virtual std::int32_t G6_ABI_CALL contains_key(abi_in_t<param_string> key, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().contains_key(create_from_abi<param_string>(key))); });
		}

		virtual std::int32_t G6_ABI_CALL try_get_record(abi_in_t<param_string> key, abi_out_t<irisviel::record> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().try_get_record(create_from_abi<param_string>(key))); });
		}

		virtual std::int32_t G6_ABI_CALL add_record(abi_in_t<irisviel::record> record) noexcept override
		{
			return abi_safe_call([&] { this->self().add_record(create_from_abi<irisviel::record>(record)); });
		}

		virtual std::int32_t G6_ABI_CALL add_records(abi_in_t<param_vector<irisviel::record>> records) noexcept override
		{
			return abi_safe_call([&] { this->self().add_records(create_from_abi<param_vector<irisviel::record>>(records)); });
		}

		virtual std::int32_t G6_ABI_CALL remove_record(abi_in_t<param_string> key) noexcept override
		{
			return abi_safe_call([&] { this->self().remove_record(create_from_abi<param_string>(key)); });
		}

		virtual std::int32_t G6_ABI_CALL remove_records(abi_in_t<param_vector<param_string>> keys) noexcept override
		{
			return abi_safe_call([&] { this->self().remove_records(create_from_abi<param_vector<param_string>>(keys)); });
		}

		virtual std::int32_t G6_ABI_CALL update_record(abi_in_t<irisviel::record> record) noexcept override
		{
			return abi_safe_call([&] { this->self().update_record(create_from_abi<irisviel::record>(record)); });
		}

		virtual std::int32_t G6_ABI_CALL update_records(abi_in_t<param_vector<irisviel::record>> records) noexcept override
		{
			return abi_safe_call([&] { this->self().update_records(create_from_abi<param_vector<irisviel::record>>(records)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_vector<float>>(feature), top_count_to_retrieve)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, float min_similarity, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_vector<float>>(feature), min_similarity)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_vector<float>> feature, float min_similarity, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_vector<float>>(feature), min_similarity, top_count_to_retrieve)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_span<const float>>(feature), top_count_to_retrieve)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, float min_similarity, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_span<const float>>(feature), min_similarity)); });
		}

		virtual std::int32_t G6_ABI_CALL search(abi_in_t<param_span<const float>> feature, float min_similarity, std::uint32_t top_count_to_retrieve, abi_out_t<param_vector<irisviel::search_result>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().search(create_from_abi<param_span<const float>>(feature), min_similarity, top_count_to_retrieve)); });
		}
	};

	template<> struct abi_adapter<irisviel::face_service>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::face_service>
		{
			void init(irisviel::face_service_implemention implementation, std::int32_t single_database_capacity, std::int32_t dimension, const param_string& working_directory) const
			{
				check_abi_result(this->self_abi().init(get_abi(static_cast<std::int32_t>(implementation)), get_abi(single_database_capacity), get_abi(dimension), get_abi(working_directory)));
			}

			void clear() const
			{
				check_abi_result(this->self_abi().clear());
			}

			void remove_all() const
			{
				check_abi_result(this->self_abi().remove_all());
			}

			std::int32_t dimension() const
			{
				std::int32_t result{};

				return (check_abi_result(this->self_abi().dimension(put_abi(result))), result);
			}

			exposing::param_string database_directory() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().database_directory(put_abi(result))), result);
			}

			param_string cache_directory() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().cache_directory(put_abi(result))), result);
			}

			void load_databases() const
			{
				check_abi_result(this->self_abi().load_databases());
			}

			std::uint64_t record_count() const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().record_count(put_abi(result))), result);
			}

			bool contains_key(const param_string& key) const
			{
				bool result{};

				return (check_abi_result(this->self_abi().contains_key(get_abi(key), put_abi(result))), result);
			}

			irisviel::record try_get_record(const param_string& key) const
			{
				irisviel::record result{ nullptr };

				return (check_abi_result(this->self_abi().try_get_record(get_abi(key), put_abi(result))), result);
			}

			void add_record(const irisviel::record& record) const
			{
				check_abi_result(this->self_abi().add_record(get_abi(record)));
			}

			void add_records(const param_vector<irisviel::record>& records) const
			{
				check_abi_result(this->self_abi().add_records(get_abi(records)));
			}

			void remove_record(const param_string& key) const
			{
				check_abi_result(this->self_abi().remove_record(get_abi(key)));
			}

			void remove_records(const param_vector<param_string>& keys) const
			{
				check_abi_result(this->self_abi().remove_records(get_abi(keys)));
			}

			void update_record(const irisviel::record& record) const
			{
				check_abi_result(this->self_abi().update_record(get_abi(record)));
			}

			void update_records(const param_vector<irisviel::record>& records) const
			{
				check_abi_result(this->self_abi().update_records(get_abi(records)));
			}

			param_vector<irisviel::search_result> search(const param_vector<float>& feature, std::uint32_t top_count_to_retrieve) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(top_count_to_retrieve), put_abi(result))), result);
			}

			param_vector<irisviel::search_result> search(const param_vector<float>& feature, float min_similarity) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(min_similarity), put_abi(result))), result);
			}

			param_vector<irisviel::search_result> search(const param_vector<float>& feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(min_similarity), get_abi(top_count_to_retrieve), put_abi(result))), result);
			}

			param_vector<irisviel::search_result> search(param_span<const float> feature, std::uint32_t top_count_to_retrieve) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(top_count_to_retrieve), put_abi(result))), result);
			}

			param_vector<irisviel::search_result> search(param_span<const float> feature, float min_similarity) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(min_similarity), put_abi(result))), result);
			}

			param_vector<irisviel::search_result> search(param_span<const float> feature, float min_similarity, std::uint32_t top_count_to_retrieve) const
			{
				param_vector<irisviel::search_result> result{ nullptr };

				return (check_abi_result(this->self_abi().search(get_abi(feature), get_abi(min_similarity), get_abi(top_count_to_retrieve), put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::irisviel
{
	struct face_service : exposing::inherits<face_service>
	{
		using inherits::inherits;
	};
}

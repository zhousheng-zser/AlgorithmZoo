#pragma once

#include "record.hpp"

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
			virtual std::int32_t clear() noexcept = 0;
			virtual std::int32_t remove_all() noexcept = 0;
			virtual std::int32_t database_directory(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t cache_directory(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t load_databases() noexcept = 0;
			virtual std::int32_t add_record(abi_in_t<irisviel::record> record) noexcept = 0;
			virtual std::int32_t add_records(abi_in_t<param_vector<irisviel::record>> records) noexcept = 0;
			virtual std::int32_t remove_record(abi_in_t<param_string> key) noexcept = 0;
			virtual std::int32_t remove_records(abi_in_t<param_vector<param_string>> keys) noexcept = 0;
			virtual std::int32_t update_record(abi_in_t<irisviel::record> record) noexcept = 0;
			virtual std::int32_t update_records(abi_in_t<param_vector<irisviel::record>> records) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::face_service> : interface_vtable_base<Derived, irisviel::face_service>
	{
		virtual std::int32_t clear() noexcept override
		{
			return abi_safe_call([&] { this->self().clear(); });
		}

		virtual std::int32_t remove_all() noexcept override
		{
			return abi_safe_call([&] { this->self().remove_all(); });
		}

		virtual std::int32_t database_directory(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().database_directory()); });
		}

		virtual std::int32_t cache_directory(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().cache_directory()); });
		}

		virtual std::int32_t load_databases() noexcept override
		{
			return abi_safe_call([&] { this->self().load_databases(); });
		}

		virtual std::int32_t add_record(abi_in_t<irisviel::record> record) noexcept override
		{
			return abi_safe_call([&] { this->self().add_record(create_from_abi<irisviel::record>(record)); });
		}

		virtual std::int32_t add_records(abi_in_t<param_vector<irisviel::record>> records) noexcept override
		{
			return abi_safe_call([&] { this->self().add_records(create_from_abi<param_vector<irisviel::record>>(records)); });
		}

		virtual std::int32_t remove_record(abi_in_t<param_string> key) noexcept override
		{
			return abi_safe_call([&] { this->self().remove_record(create_from_abi<param_string>(key)); });
		}

		virtual std::int32_t remove_records(abi_in_t<param_vector<param_string>> keys) noexcept override
		{
			return abi_safe_call([&] { this->self().remove_records(create_from_abi<param_vector<param_string>>(keys)); });
		}

		virtual std::int32_t update_record(abi_in_t<irisviel::record> record) noexcept override
		{
			return abi_safe_call([&] { this->self().update_record(create_from_abi<irisviel::record>(record)); });
		}

		virtual std::int32_t update_records(abi_in_t<param_vector<irisviel::record>> records) noexcept override
		{
			return abi_safe_call([&] { this->self().update_records(create_from_abi < param_vector<irisviel::record>(records)); });
		}
	};

	template<> struct abi_adapter<irisviel::face_service>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::face_service>
		{
			void clear() const
			{
				check_abi_result(this->self_abi().clear());
			}

			void remove_all() const
			{
				check_abi_result(this->self_abi().remove_all());
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

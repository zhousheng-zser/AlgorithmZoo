#pragma once

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct database_record;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<irisviel::database_record>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "D0869CF1-689F-4310-B7FB-0700F0AA5E68" };

		struct type : abi_unknown_object
		{
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::database_record> : interface_vtable_base<Derived, irisviel::database_record>
	{

	};

	template<> struct abi_adapter<irisviel::database_record>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::database_record>
		{

		};
	};
}

namespace glasssix::irisviel
{
	struct database_record : exposing::inherits<database_record>
	{
		using inherits::inherits;
	};
}

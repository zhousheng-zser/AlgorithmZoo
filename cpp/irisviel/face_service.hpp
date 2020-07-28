#pragma once

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
			virtual std::int32_t remove(abi_in_t<param_string> key) noexcept = 0;
			virtual std::int32_t remove(abi_in_t<param_span<param_string>> keys) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::face_service> : interface_vtable_base<Derived, irisviel::face_service>
	{
		
	};

	template<> struct abi_adapter<irisviel::face_service>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::face_service>
		{
			
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

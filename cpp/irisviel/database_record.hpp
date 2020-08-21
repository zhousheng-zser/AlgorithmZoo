#pragma once

#include "dynamic_buffer.hpp"

#include <memory>
#include <cstddef>
#include <string_view>

#include <abi/param_span.hpp>

namespace glasssix
{
	namespace irisviel
	{
		struct database_record : dynamic_buffer, std::enable_shared_from_this<database_record>
		{
			virtual int dimension() const noexcept = 0;
			virtual bool active() const noexcept = 0;
			virtual void active(bool value) noexcept = 0;
			virtual std::string_view key() const noexcept = 0;
			virtual void key(std::string_view value) noexcept = 0;
			virtual exposing::param_span<const float> feature() const noexcept = 0;
			virtual void feature(exposing::param_span<const float> value) noexcept = 0;
			virtual std::size_t feature_offset() const noexcept = 0;
			std::shared_ptr<database_record> shared();

			static std::size_t record_size(int dimension) noexcept;
			static std::size_t feature_offset(int dimension) noexcept;
			static std::shared_ptr<database_record> create(int dimension);
			static std::shared_ptr<database_record> create(int dimension, std::uint8_t* ptr);
			static std::shared_ptr<database_record> create_ref(int dimension, std::uint8_t* ptr);
			static bool key_equals(const char* left, const char* right);
			static bool key_equals(const database_record& left, const database_record& right);
		};
	}
}

#pragma once

#include "memory_mapping.hpp"
#include "dynamic_buffer.hpp"

#include <string>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <string_view>

namespace glasssix
{
	namespace irisviel
	{
		class memory_mapping_operator
		{
		public:
			memory_mapping_operator(std::string_view file_path, std::size_t max_size);
			virtual ~memory_mapping_operator() = default;
			const std::uint8_t* const_data() const noexcept;
			std::uint8_t* mutable_data() const noexcept;
			std::size_t size() const noexcept;
			std::string path() const;
			void save_changes() noexcept;
			void mark_for_deletion() noexcept;
			std::uint8_t* locate_bytes(std::size_t offset) const noexcept;
			std::uint8_t* locate_element_bytes(std::size_t index, std::size_t element_size) const noexcept;
			void write_bytes(std::size_t offset, const std::uint8_t* buffer, std::size_t size);
			void write_element_bytes(std::size_t index, const std::uint8_t* buffer, std::size_t element_size);
			void get_dynamic_buffer(std::size_t index, dynamic_buffer& buffer) const;
			void write_dynamic_buffer(std::size_t index, const dynamic_buffer& buffer);

			template<typename Object>
			Object* locate_element_absolutely(std::size_t offset) const noexcept
			{
				return reinterpret_cast<Object*>(locate_bytes(offset));
			}

			template<typename Object>
			Object* locate_element(std::size_t index) const noexcept
			{
				return reinterpret_cast<Object*>(locate_element_bytes(index, sizeof(Object)));
			}

			template<typename Object>
			void write_element_absolutely(std::size_t offset, Object&& buffer)
			{
				write_bytes(offset, reinterpret_cast<const std::uint8_t*>(&std::forward<Object>(buffer)), sizeof(Object&&));
			}

			template<typename Object>
			void write_element(std::size_t index, Object&& buffer)
			{
				write_element_bytes(index, reinterpret_cast<const std::uint8_t*>(&std::forward<Object>(buffer)), sizeof(Object&&));
			}
		private:
			memory_mapping mapping_;
		};
	}
}

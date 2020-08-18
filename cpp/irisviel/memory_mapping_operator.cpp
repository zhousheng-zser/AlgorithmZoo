#include "memory_mapping_operator.hpp"

#include <cstring>
#include <stdexcept>

namespace glasssix
{
	namespace irisviel
	{
		memory_mapping_operator::memory_mapping_operator(std::string_view file_path, std::size_t max_size) : mapping_{ file_path, max_size }
		{
			if (!mapping_)
			{
				throw std::runtime_error{ "Fail to map the file into the memory." };
			}
		}

		const std::uint8_t* memory_mapping_operator::const_data() const noexcept
		{
			return mapping_.data();
		}

		std::uint8_t* memory_mapping_operator::mutable_data() const noexcept
		{
			return mapping_.data();
		}

		std::size_t memory_mapping_operator::size() const noexcept
		{
			return mapping_.size();
		}

		std::string memory_mapping_operator::path() const
		{
			return mapping_.path();
		}

		void memory_mapping_operator::save_changes() noexcept
		{
			mapping_.flush();
		}

		void memory_mapping_operator::mark_for_deletion() noexcept
		{
			mapping_.mark_for_deletion();
		}

		std::uint8_t* memory_mapping_operator::locate_bytes(std::size_t offset) const noexcept
		{
			return offset < mapping_.size() ? mapping_.data() + offset : nullptr;
		}

		std::uint8_t* memory_mapping_operator::locate_element_bytes(std::size_t index, std::size_t element_size)  const noexcept
		{
			return (index + 1) * element_size <= mapping_.size() ? mapping_.data() + index * element_size : nullptr;
		}

		void memory_mapping_operator::write_bytes(std::size_t offset, const std::uint8_t* buffer, std::size_t size)
		{
			if (offset + size > mapping_.size())
			{
				throw std::out_of_range{ "The offset is out of range." };
			}

			std::memmove(mapping_.data() + offset, buffer, size);
		}

		void memory_mapping_operator::write_element_bytes(std::size_t index, const std::uint8_t* buffer, std::size_t element_size)
		{
			if ((index + 1) * element_size > mapping_.size())
			{
				throw std::out_of_range{ "The index is out of range." };
			}

			std::memmove(mapping_.data() + index * element_size, buffer, element_size);
		}

		void memory_mapping_operator::get_dynamic_buffer(std::size_t index, dynamic_buffer& buffer) const
		{
			if (auto ptr = locate_element_bytes(index, buffer.size()))
			{
				std::memmove(buffer.data(), ptr, buffer.size());
			}
		}

		void memory_mapping_operator::write_dynamic_buffer(std::size_t index, const dynamic_buffer& buffer)
		{
			write_element_bytes(index, buffer.data(), buffer.size());
		}
	}
}

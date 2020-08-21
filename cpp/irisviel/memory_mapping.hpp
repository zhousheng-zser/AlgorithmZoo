#pragma once

#include <string>
#include <cstddef>
#include <string_view>

namespace glasssix
{
	class memory_mapping
	{
	public:
		class impl;

		memory_mapping(std::string_view path, std::size_t size) noexcept;
		virtual ~memory_mapping();
		operator bool() const noexcept;
		std::uint8_t* data() const noexcept;
		std::size_t size() const noexcept;
		std::string path() const;
		void flush() noexcept;
		void mark_for_deletion() noexcept;
	private:
		impl* impl_;
	};
}

#include "memory_mapping.hpp"
#include "filesystem_utils.hpp"

#include <algorithm>

#ifdef __linux__
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LINUX_FAILURE(x) ((x) < 0)
#define LINUX_SUCCESS(x) ((x) > -1)

#elif defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#endif

namespace glasssix
{
#ifdef __linux__
	namespace
	{
		constexpr int linux_general_error_code = -1;
	}

	class memory_mapping::impl
	{
	public:
		using linux_stat_type = struct stat64;

		impl(std::string_view path, std::size_t size) noexcept : path_{ path }, size_{ size }, data_{}, file_descriptor_{}, mark_for_deletion_{}
		{
			file_descriptor_ = open64(path_.c_str(), O_RDWR);

			if (LINUX_FAILURE(file_descriptor_))
			{
				return;
			}

			linux_stat_type status;

			if (LINUX_FAILURE(fstat64(file_descriptor_, &status)))
			{
				return;
			}

			// Sets the size of the file.
			size_ = std::max(static_cast<std::size_t>(status.st_size), size);

			if (LINUX_FAILURE(ftruncate64(file_descriptor_, size_)))
			{
				return;
			}

			data_ = static_cast<std::uint8_t*>(mmap64(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_, 0));
		}

		~impl()
		{
			if (*this)
			{
				flush();
				munmap(data_, size_);
				data_ = nullptr;
			}

			if (LINUX_SUCCESS(file_descriptor_))
			{
				close(file_descriptor_);
				file_descriptor_ = linux_general_error_code;
			}

			if (mark_for_deletion_)
			{
				utils::safe_remove_file(path_);
			}
		}

		operator bool() const noexcept
		{
			return data_ && data_ != MAP_FAILED;
		}

		std::uint8_t* data() const noexcept
		{
			return data_;
		}

		std::size_t size() const noexcept
		{
			return size_;
		}

		std::string path() const noexcept
		{
			return path_;
		}

		void flush() noexcept
		{
			if (*this)
			{
				msync(data_, size_, MS_SYNC);
			}
		}

		void mark_for_deletion() noexcept
		{
			mark_for_deletion_ = true;
		}
	private:
		std::string path_;
		std::size_t size_;
		std::uint8_t* data_;
		int file_descriptor_;
		bool mark_for_deletion_;
	};
#elif defined(_MSC_VER)
	class memory_mapping::impl
	{
	public:
		impl(std::string_view path, std::size_t size) noexcept : path_{ path }, size_{ size }, data_{}, file_handle_{}, mapping_handle_{}, mark_for_deletion_{}
		{
			file_handle_ = CreateFileA(path_.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

			if (!file_handle_)
			{
				return;
			}

			LARGE_INTEGER file_size;

			if (!GetFileSizeEx(file_handle_, &file_size))
			{
				return;
			}

			// Sets the size of the file.
			size_ = std::max(static_cast<std::size_t>(file_size.QuadPart), size);
			file_size.QuadPart = size_;

			if (!SetFilePointerEx(file_handle_, file_size, nullptr, FILE_BEGIN))
			{
				return;
			}

			// Extends the file.
			if (!SetEndOfFile(file_handle_))
			{
				return;
			}

			mapping_handle_ = CreateFileMappingA(file_handle_, nullptr, PAGE_READWRITE, 0, 0, nullptr);

			if (!mapping_handle_)
			{
				return;
			}

			data_ = static_cast<std::uint8_t*>(MapViewOfFileEx(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, 0, nullptr));
		}

		~impl()
		{
			if (data_)
			{
				flush();
				UnmapViewOfFile(data_);
				data_ = nullptr;
			}

			if (mapping_handle_)
			{
				CloseHandle(mapping_handle_);
				mapping_handle_ = nullptr;
			}

			if (file_handle_)
			{
				CloseHandle(file_handle_);
				file_handle_ = nullptr;
			}

			if (mark_for_deletion_)
			{
				utils::safe_remove_file(path_);
			}
		}

		operator bool() const noexcept
		{
			return data_;
		}

		std::uint8_t* data() const noexcept
		{
			return data_;
		}

		std::size_t size() const noexcept
		{
			return size_;
		}

		std::string path() const noexcept
		{
			return path_;
		}

		void flush() noexcept
		{
			if (data_)
			{
				FlushViewOfFile(data_, 0);
			}
		}

		void mark_for_deletion() noexcept
		{
			mark_for_deletion_ = true;
		}
	private:
		std::string path_;
		std::size_t size_;
		std::uint8_t* data_;
		HANDLE file_handle_;
		HANDLE mapping_handle_;
		bool mark_for_deletion_;
	};
#endif
	memory_mapping::memory_mapping(std::string_view path, std::size_t size) noexcept : impl_{ new impl{ path, size } }
	{
	}

	memory_mapping::~memory_mapping()
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	memory_mapping::operator bool() const noexcept
	{
		return *impl_;
	}

	std::uint8_t* memory_mapping::data() const noexcept
	{
		return impl_->data();
	}

	std::size_t memory_mapping::size() const noexcept
	{
		return impl_->size();
	}

	std::string memory_mapping::path() const
	{
		return impl_->path();
	}

	void memory_mapping::flush() noexcept
	{
		impl_->flush();
	}

	void memory_mapping::mark_for_deletion() noexcept
	{
		impl_->mark_for_deletion();
	}
}

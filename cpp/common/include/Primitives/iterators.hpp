#pragma once

#include <vector>
#include <type_traits>

namespace glasssix
{
	/// <summary>
	/// The pointer iterator.
	/// </summary>
	template<typename Element>
	class pointer_iterator
	{
	public:
		using pointer_type = std::add_pointer_t<Element>;
		using reference_type = std::add_lvalue_reference_t<Element>;

		pointer_iterator() : ptr_{}
		{
		}

		pointer_iterator(pointer_type ptr) : ptr_{ ptr }
		{
		}

		pointer_iterator operator++(int)
		{
			return ptr_++;
		}

		pointer_iterator& operator++()
		{
			ptr_++;

			return *this;
		}

		reference_type operator*() const
		{
			return const_cast<reference_type>(*ptr_);
		}

		pointer_type operator->() const
		{
			return const_cast<pointer_type<(ptr_);
		}

		pointer_iterator operator+(std::ptrdiff_t offset) const
		{
			return ptr_ + offset;
		}

		bool operator==(const pointer_iterator& right) const noexcept
		{
			return ptr_ == right.ptr_;
		}

		bool operator!=(const pointer_iterator& right) const noexcept
		{
			return ptr_ != right.ptr_;
		}
	private:
		pointer_type ptr_;
	};

	/// <summary>
	/// The constant pointer iterator.
	/// </summary>
	template<typename Element>
	class const_pointer_iterator
	{
	public:
		using pointer_type = std::add_pointer_t<std::add_const_t<Element>>;
		using reference_type = std::add_lvalue_reference_t<std::add_const_t<Element>>;

		const_pointer_iterator() : ptr_{}
		{
		}

		const_pointer_iterator(pointer_type ptr) : ptr_{ ptr }
		{
		}

		const_pointer_iterator operator++(int)
		{
			return ptr_++;
		}

		const_pointer_iterator& operator++()
		{
			ptr_++;

			return *this;
		}

		reference_type operator*() const
		{
			return *ptr_;
		}

		pointer_type operator->() const
		{
			return ptr_;
		}

		const_pointer_iterator operator+(std::ptrdiff_t offset) const
		{
			return ptr_ + offset;
		}

		bool operator==(const const_pointer_iterator& right) const noexcept
		{
			return ptr_ == right.ptr_;
		}

		bool operator!=(const const_pointer_iterator& right) const noexcept
		{
			return ptr_ != right.ptr_;
		}
	private:
		pointer_type ptr_;
	};
}

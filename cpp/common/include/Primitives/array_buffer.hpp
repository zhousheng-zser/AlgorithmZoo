#pragma once

#include "iterators.hpp"
#include "heap_allocator.hpp"

#include <cstddef>
#include <type_traits>

namespace glasssix
{
	namespace memory
	{
		/// <summary>
		/// Supports a Glasssix-owned simple buffer individually from the standard library.
		/// </summary>
		template<typename Element>
		class array_buffer
		{
		public:
			using iterator_type = pointer_iterator<Element>;
			using const_iterator_type = const_pointer_iterator<Element>;

			/// <summary>
			/// Creates an instance with the default initial capacity.
			/// </summary>
			array_buffer() : array_buffer{ initial_capacity_ }
			{
			}

			/// <summary>
			/// Creates an instance with a given capacity.
			/// </summary>
			/// <param name="capacity">The capacity</param>
			array_buffer(std::size_t capacity) : position_{}, capacity_{ capacity }, buffer_{ allocator_.allocate(capacity) }
			{
			}

			virtual ~array_buffer()
			{
				if (buffer_ != nullptr)
				{
					for (std::size_t i = 0; i < position_; i++)
					{
						allocator_.destroy(buffer_ + i);
					}

					if (capacity_ != 0)
					{
						allocator_.deallocate(buffer_, capacity_);
						capacity_ = 0;
					}

					buffer_ = nullptr;
				}
			}

			/// <summary>
			/// Gets the size of the buffer.
			/// </summary>
			/// <returns>The size</returns>
			std::size_t size() const noexcept
			{
				return position_;
			}

			/// <summary>
			/// Gets the capacity of the buffer.
			/// </summary>
			/// <returns>The capacity</returns>
			std::size_t capacity() const noexcept
			{
				return capacity_;
			}

			/// <summary>
			/// Gets the item at an index.
			/// </summary>
			/// <param name="index">The index</param>
			/// <returns>The item</returns>
			Element& operator[](std::size_t index)
			{
				if (index >= position_)
				{
					glasssix_terminate();
				}

				return buffer_[index];
			}

			/// <summary>
			/// Gets the item at an index.
			/// </summary>
			/// <param name="index">The index</param>
			/// <returns>The item</returns>
			const Element& operator[](std::size_t index) const
			{
				if (index >= position_)
				{
					glasssix_terminate();
				}

				return buffer_[index];
			}

			/// <summary>
			/// Gets the iterator at the beginning of the elements.
			/// </summary>
			/// <returns>The iterator</returns>
			iterator_type begin()
			{
				return buffer_;
			}

			/// <summary>
			/// Gets the iterator at the end of the elements.
			/// </summary>
			/// <returns>The iterator</returns>
			iterator_type end()
			{
				return buffer_ != nullptr ? buffer_ + position_ : nullptr;
			}

			/// <summary>
			/// Gets the constant iterator at the beginning of the elements.
			/// </summary>
			/// <returns>The iterator</returns>
			const_iterator_type begin() const
			{
				return cbegin();
			}

			/// <summary>
			/// Gets the constant iterator at the end of the elements.
			/// </summary>
			/// <returns>The iterator</returns>
			const_iterator_type end() const
			{
				return cend();
			}

			/// <summary>
			/// Gets the constant iterator at the beginning of the elements.
			/// </summary>
			/// <returns>The iterator</returns>
			const_iterator_type cbegin() const
			{
				return buffer_;
			}

			/// <summary>
			/// Gets the constant iterator at the end of the elements.
			/// </summary>
			/// <returns>The iterator</returns>

			const_iterator_type cend() const
			{
				return buffer_ != nullptr ? buffer_ + position_ : nullptr;
			}

			/// <summary>
			/// Constructs an item and emplaces it directly.
			/// </summary>
			/// <param name="args">The arguments of the constructor</param>
			/// <returns>The reference of the instance</returns>
			template<typename... Args>
			auto emplace_back(Args&&... args) -> std::enable_if_t<std::is_constructible<Element, Args...>::value, array_buffer&>
			{
				expand_buffer_core();

				std::size_t position = position_++;

				// Constructs the new object.
				allocator_.construct(buffer_ + position, std::forward<Args>(args...));

				return buffer_[position];
			}
		private:
			void expand_buffer_core()
			{
				if (position_ >= capacity_)
				{
					auto new_buffer = new Element[capacity_ *= expanding_factor_];

					if (buffer_ != nullptr)
					{
						std::memcpy(new_buffer, buffer_, position_);
						allocator_.deallocate(buffer_, capacity_);
					}

					buffer_ = new_buffer;
				}
			}
		private:
			Element* buffer_;
			std::size_t position_;
			std::size_t capacity_;
			heap_allocator<Element> allocator_;
			static constexpr std::size_t initial_capacity_ = 1;
			static constexpr std::size_t expanding_factor_ = 2;
		};
	}
}

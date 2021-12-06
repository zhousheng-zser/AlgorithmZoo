#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "exceptions.hpp"
#include "param_span.hpp"
#include "iterable_object.hpp"

#include <vector>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <iterator>
#include <algorithm>
#include <type_traits>

namespace glasssix::exposing
{
	template<typename T>
	struct param_vector;
}

namespace glasssix::exposing::impl
{
	/// <summary>
	/// The ABI of a param_vector.
	/// </summary>
	template<typename T>
	struct abi<param_vector<T>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "DCB2A5A5-1D17-4E0A-83C2-640912AECD25" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL empty(abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL size(abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL at(std::uint64_t index, abi_out_t<T> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_at(std::uint64_t index, abi_in_t<T> item) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL push_back(abi_in_t<T> item) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL remove_at(std::uint64_t index) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL insert_at(std::uint64_t index, abi_in_t<T> item) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL contains(abi_in_t<T> item, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL clear() noexcept = 0;
			virtual std::int32_t G6_ABI_CALL resize(std::uint64_t size) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL reserve(std::uint64_t capacity) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL copy_from(abi_in_t<param_span<const T>> data, std::uint64_t index) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL copy_to(std::uint64_t index, abi_in_t<param_span<T>> data) noexcept = 0;
		};
	};

	/// <summary>
	/// The vtable of a param_vector.
	/// </summary>
	template<typename Derived, typename T>
	struct interface_vtable<Derived, param_vector<T>> : interface_vtable_base<Derived, param_vector<T>>
	{
		virtual std::int32_t G6_ABI_CALL empty(abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().empty()); });
		}

		virtual std::int32_t G6_ABI_CALL size(abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().size()); });
		}

		virtual std::int32_t G6_ABI_CALL at(std::uint64_t index, abi_out_t<T> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().at(index)); });
		}

		virtual std::int32_t G6_ABI_CALL set_at(std::uint64_t index, abi_in_t<T> item) noexcept override
		{
			return abi_safe_call([&] { this->self().set_at(index, create_from_abi<T>(item)); });
		}

		virtual std::int32_t G6_ABI_CALL push_back(abi_in_t<T> item) noexcept override
		{
			return abi_safe_call([&] { this->self().push_back(create_from_abi<T>(item)); });
		}

		virtual std::int32_t G6_ABI_CALL remove_at(std::uint64_t index) noexcept override
		{
			return abi_safe_call([&] { this->self().remove_at(index); });
		}

		virtual std::int32_t G6_ABI_CALL insert_at(std::uint64_t index, abi_in_t<T> item) noexcept override
		{
			return abi_safe_call([&] { this->self().insert_at(index, create_from_abi<T>(item)); });
		}

		virtual std::int32_t G6_ABI_CALL contains(abi_in_t<T> item, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = this->self().contains(create_from_abi<T>(item)); });
		}

		virtual std::int32_t G6_ABI_CALL clear() noexcept override
		{
			return abi_safe_call([&] { this->self().clear(); });
		}

		virtual std::int32_t G6_ABI_CALL resize(std::uint64_t size) noexcept override
		{
			return abi_safe_call([&] { this->self().resize(size); });
		}

		virtual std::int32_t G6_ABI_CALL reserve(std::uint64_t capacity) noexcept override
		{
			return abi_safe_call([&] { this->self().reserve(capacity); });
		}

		virtual std::int32_t G6_ABI_CALL copy_from(abi_in_t<param_span<const T>> data, std::uint64_t index) noexcept override
		{
			return abi_safe_call([&] { this->self().copy_from(create_from_abi<param_span<const T>>(data), index); });
		}

		virtual std::int32_t G6_ABI_CALL copy_to(std::uint64_t index, abi_in_t<param_span<T>> data) noexcept override
		{
			return abi_safe_call([&] { this->self().copy_to(index, create_from_abi<param_span<T>>(data)); });
		}
	};

	/// <summary>
	/// The ABI adapter of a param_vector.
	/// </summary>
	template<typename T>
	struct abi_adapter<param_vector<T>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, param_vector<T>>
		{
			T operator[](std::uint64_t index) const
			{
				return at(index);
			}

			bool empty() const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().empty(put_abi(result))), result);
			}

			std::uint64_t size() const
			{
				std::uint64_t result = 0;

				return (check_abi_result(this->self_abi().size(put_abi(result))), result);
			}

			T at(std::uint64_t index) const
			{
				T result{ null_value_v<T> };

				return (check_abi_result(this->self_abi().at(index, put_abi(result))), result);
			}

			void set_at(std::uint64_t index, const T& item) const
			{
				check_abi_result(this->self_abi().set_at(index, get_abi(item)));
			}

			void push_back(const T& item) const
			{
				check_abi_result(this->self_abi().push_back(get_abi(item)));
			}

			void remove_at(std::uint64_t index) const
			{
				check_abi_result(this->self_abi().remove_at(get_abi(index)));
			}

			void insert_at(std::uint64_t index, const T& element) const
			{
				check_abi_result(this->self_abi().insert_at(get_abi(index), get_abi(element)));
			}

			bool contains(const T& item) const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().contains(get_abi(item), put_abi(result))), result);
			}

			void clear() const
			{
				check_abi_result(this->self_abi().clear());
			}

			void resize(std::uint64_t size) const
			{
				check_abi_result(this->self_abi().resize(get_abi(size)));
			}

			void reserve(std::uint64_t capacity) const
			{
				check_abi_result(this->self_abi().reserve(get_abi(capacity)));
			}

			void copy_from(param_span<const T> data, std::uint64_t index) const
			{
				check_abi_result(this->self_abi().copy_from(get_abi(data), get_abi(index)));
			}

			void copy_to(std::uint64_t index, param_span<T> data) const
			{
				check_abi_result(this->self_abi().copy_to(get_abi(index), get_abi(data)));
			}
		};
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// A mutable vector.
	/// </summary>
	template<typename T>
	struct param_vector : inherits<param_vector<T>, iterable_object<T>>
	{
		using inherits<param_vector<T>, iterable_object<T>>::inherits;
	};
}

namespace glasssix::exposing::impl
{
	/// <summary>
	/// An implementation of a param_vector.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	template<typename T>
	class param_vector_impl : public implements<param_vector_impl<T>, param_vector<T>>
	{
	public:
		param_vector_impl()
		{
		}

		param_vector_impl(param_span<const T> data) : buffer_(data.data(), data.data() + data.size())
		{
		}

		param_vector_impl(param_span<T> data) : buffer_(data.data(), data.data() + data.size())
		{
		}

		template<typename... Args, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<Args, T>...>>>
		param_vector_impl(Args&&... args) : buffer_{ std::forward<Args>(args)... }
		{
		}

		bool empty() const
		{
			return buffer_.empty();
		}

		std::uint64_t size() const
		{
			return buffer_.size();
		}

		T at(std::uint64_t index) const
		{
			return buffer_[index];
		}

		void set_at(std::uint64_t index, const T& item)
		{
			buffer_[index] = item;
		}

		void push_back(const T& item)
		{
			buffer_.emplace_back(item);
		}

		void remove_at(std::uint64_t index)
		{
			buffer_.erase(buffer_.begin() + index);
		}

		void insert_at(std::uint64_t index, const T& item)
		{
			buffer_.insert(buffer_.begin() + index, item);
		}

		void clear()
		{
			buffer_.clear();
		}

		bool contains(const T& item) const
		{
			return std::any_of(buffer_.begin(), buffer_.end(), [&](const T& inner) { return item == inner; });
		}

		object_iterator<T> get_iterator() const
		{
			return make_as_first<object_iterator_impl>(*this);
		}

		void resize(std::uint64_t size)
		{
			buffer_.resize(static_cast<std::size_t>(size));
		}

		void reserve(std::uint64_t capacity)
		{
			buffer_.reserve(static_cast<std::size_t>(capacity));
		}

		void copy_from(param_span<const T> data, std::uint64_t index)
		{
			if (data.size() > buffer_.size() || index > buffer_.size() - data.size())
			{
				throw abi_out_of_bounds{};
			}

			std::copy(data.begin(), data.end(), buffer_.begin() + index);
		}

		void copy_to(std::uint64_t index, param_span<T> data)
		{
			if (data.size() > buffer_.size() || index > buffer_.size() - data.size())
			{
				throw abi_out_of_bounds{};
			}

			std::copy(buffer_.begin() + index, buffer_.begin() + index + data.size(), data.begin());
		}

	private:
		/// <summary>
		/// Implements an object iterator.
		/// </summary>
		class object_iterator_impl : public implements<object_iterator_impl, object_iterator<T>>
		{
		public:
			object_iterator_impl(const param_vector_impl& impl) : index_{}, impl_{ impl }
			{
			}

			T current() const
			{
				return impl_.at(index_);
			}

			bool valid() const
			{
				return index_ < impl_.size();
			}

			bool move_to_next()
			{
				return ++index_ < impl_.size();
			}
		private:
			std::uint64_t index_;
			const param_vector_impl& impl_;
		};

		std::vector<T> buffer_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Creates a N-dimensional param_vector.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <returns>The result</returns>
	template<typename T, std::size_t Dimension = 1, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<T>>>>
	auto make_param_vector()
	{
		using element_type = meta::make_multidimensional_container_t<param_vector, T, Dimension - 1>;

		return make_as_first<impl::param_vector_impl<element_type>>();
	}

	/// <summary>
	/// Creates a one-dimensional param_vector.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <typeparam name="...Args">The types of the initializer</typeparam>
	/// <param name="...args">The initializer</param>
	/// <returns>The result</returns>
	template<typename T, typename... Args, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<T>, std::is_convertible<Args, T>...>>>
	auto make_param_vector(Args&&... args)
	{
		return make_as_first<impl::param_vector_impl<T>>(std::forward<Args>(args)...);
	}

	/// <summary>
	/// Creates a one-dimensional param_vector from a span.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <typeparam name="...Args">The types of the initializer</typeparam>
	/// <param name="...args">The initializer</param>
	/// <param name="data">The data span</param>
	/// <returns>The result</returns>
	template<typename T, typename = std::enable_if_t<impl::has_abi_type_v<T>>>
	auto make_param_vector(param_span<T> data)
	{
		return make_as_first<impl::param_vector_impl<T>>(data);
	}

	/// <summary>
	/// Creates a one-dimensional param_vector from a read-only span.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <typeparam name="...Args">The types of the initializer</typeparam>
	/// <param name="...args">The initializer</param>
	/// <param name="data">The data span</param>
	/// <returns>The result</returns>
	template<typename T, typename = std::enable_if_t<impl::has_abi_type_v<T>>>
	auto make_param_vector(param_span<const T> data)
	{
		return make_as_first<impl::param_vector_impl<T>>(data);
	}
}

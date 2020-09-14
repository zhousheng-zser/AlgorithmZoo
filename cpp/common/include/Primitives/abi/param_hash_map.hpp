#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "exceptions.hpp"
#include "param_string.hpp"
#include "iterable_object.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <initializer_list>

namespace glasssix::exposing
{
	template<typename Key, typename Value>
	struct param_pair;

	template<typename Key, typename Value>
	struct param_hash_map;
}

namespace glasssix::exposing::impl
{
	/// <summary>
	/// The ABI of a param_pair.
	/// </summary>
	template<typename Key, typename Value>
	struct abi<param_pair<Key, Value>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "77FBFA1B-0E03-4D44-BC66-268C676DDC23" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL key(abi_out_t<Key> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL value(abi_out_t<Value> result) noexcept = 0;
		};
	};

	/// <summary>
	/// The ABI of a param_hash_map.
	/// </summary>
	template<typename Key, typename Value>
	struct abi<param_hash_map<Key, Value>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "5218106E-2AC9-438F-81CF-A1ED421878F6" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL empty(abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL size(abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get_value(abi_in_t<Key> key, abi_out_t<Value> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL try_get_value(abi_in_t<Key> key, abi_out_t<Value> value, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_or_update(abi_in_t<Key> key, abi_in_t<Value> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL contains(abi_in_t<Key> key, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL remove(abi_in_t<Key> key) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL clear() noexcept = 0;
		};
	};

	/// <summary>
	/// The vtable of a param_pair.
	/// </summary>
	template<typename Derived, typename Key, typename Value>
	struct interface_vtable<Derived, param_pair<Key, Value>> : interface_vtable_base<Derived, param_pair<Key, Value>>
	{
		virtual std::int32_t G6_ABI_CALL key(abi_out_t<Key> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().key()); });
		}

		virtual std::int32_t G6_ABI_CALL value(abi_out_t<Value> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().value()); });
		}
	};

	/// <summary>
	/// The vtable of a param_hash_map.
	/// </summary>
	template<typename Derived, typename Key, typename Value>
	struct interface_vtable<Derived, param_hash_map<Key, Value>> : interface_vtable_base<Derived, param_hash_map<Key, Value>>
	{
		virtual std::int32_t G6_ABI_CALL empty(abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().empty()); });
		}

		virtual std::int32_t G6_ABI_CALL size(abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().size()); });
		}

		virtual std::int32_t G6_ABI_CALL get_value(abi_in_t<Key> key, abi_out_t<Value> value) noexcept override
		{
			return abi_safe_call([&] { *value = detach_abi(this->self().get_value(create_from_abi<Key>(key))); });
		}

		virtual std::int32_t G6_ABI_CALL try_get_value(abi_in_t<Key> key, abi_out_t<Value> value, abi_out_t<bool> result) noexcept override
		{
			Value tmp{ null_value_v<Value> };
			abi_result result_code;

			return (result_code = abi_safe_call([&] { *result = this->self().try_get_value(create_from_abi<Key>(key), tmp); }), *value = detach_abi(tmp), result_code);
		}

		virtual std::int32_t G6_ABI_CALL add_or_update(abi_in_t<Key> key, abi_in_t<Value> value) noexcept override
		{
			return abi_safe_call([&] { this->self().add_or_update(create_from_abi<Key>(key), create_from_abi<Value>(value)); });
		}

		virtual std::int32_t G6_ABI_CALL contains(abi_in_t<Key> key, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = this->self().contains(create_from_abi<Key>(key)); });
		}

		virtual std::int32_t G6_ABI_CALL remove(abi_in_t<Key> key) noexcept override
		{
			return abi_safe_call([&] { this->self().remove(create_from_abi<Key>(key)); });
		}

		virtual std::int32_t G6_ABI_CALL clear() noexcept override
		{
			return abi_safe_call([&] { this->self().clear(); });
		}
	};

	/// <summary>
	/// The ABI adapter of a param_hash_map.
	/// </summary>
	template<typename Key, typename Value>
	struct abi_adapter<param_hash_map<Key, Value>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, param_hash_map<Key, Value>>
		{
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

			Value get_value(const Key& key) const
			{
				Value result{ null_value_v<Value> };

				return (check_abi_result(this->self_abi().get_value(get_abi(key), put_abi(result))), result);
			}

			bool try_get_value(const Key& key, Value& value) const
			{
				bool result = false;

				return (value = null_value_v<Value>, check_abi_result(this->self_abi().try_get_value(get_abi(key), put_abi(value), put_abi(result))), result);
			}

			void add_or_update(const Key& key, const Value& value) const
			{
				check_abi_result(this->self_abi().add_or_update(get_abi(key), get_abi(value)));
			}

			bool contains(const Key& key) const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().contains(get_abi(key), put_abi(result))));
			}

			void remove(const Key& key) const
			{
				check_abi_result(this->self_abi().remove(get_abi(key)));
			}

			void clear() const
			{
				check_abi_result(this->self_abi().clear());
			}
		};
	};

	/// <summary>
	/// The ABI adapter of a param_pair.
	/// </summary>
	template<typename Key, typename Value>
	struct abi_adapter<param_pair<Key, Value>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, param_pair<Key, Value>>
		{
			Key key() const
			{
				Key result{ null_value_v<Key> };

				return (check_abi_result(this->self_abi().key(put_abi(result))), result);
			}

			Value value() const
			{
				Value result{ null_value_v<Value> };

				return (check_abi_result(this->self_abi().value(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// A key-value pair.
	/// </summary>
	template<typename Key, typename Value>
	struct param_pair : inherits<param_pair<Key, Value>>
	{
		using inherits<param_pair<Key, Value>>::inherits;
	};

	/// <summary>
	/// A hash map.
	/// </summary>
	template<typename Key, typename Value>
	struct param_hash_map : inherits<param_hash_map<Key, Value>, iterable_object<param_pair<Key, Value>>>
	{
		using inherits<param_hash_map<Key, Value>, iterable_object<param_pair<Key, Value>>>::inherits;
	};
}

namespace glasssix::exposing::impl
{
	template<typename Key, typename Value>
	class param_pair_impl : public implements<param_pair_impl<Key, Value>, param_pair<Key, Value>>
	{
	public:
		param_pair_impl() : key_{}, value_{}
		{
		}

		template<typename First, typename Second, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<First, Key>, std::is_convertible<Second, Value>>>>
		param_pair_impl(First&& key, Second&& value) : key_{ std::forward<First>(key) }, value_{ std::forward<Second>(value) }
		{
		}

		Key key() const
		{
			return key_;
		}

		Value value() const
		{
			return value_;
		}
	private:
		Key key_;
		Value value_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Creates a key-value pair.
	/// </summary>
	/// <typeparam name="Key">The key type</typeparam>
	/// <typeparam name="Value">The value type</typeparam>
	/// <param name="key">The key</param>
	/// <param name="value">The value</param>
	/// <returns>The pair</returns>
	template<typename Key, typename Value, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<std::decay_t<Key>>, impl::has_abi_type<std::decay_t<Value>>>>>
	auto make_param_pair(Key&& key, Value&& value)
	{
		return make_as_first<impl::param_pair_impl<std::decay_t<Key>, std::decay_t<Value>>>(std::forward<Key>(key), std::forward<Value>(value));
	}
}

namespace glasssix::exposing::impl
{
	template<typename Key, typename Value, typename Hasher = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
	class param_hash_map_impl : public implements<param_hash_map_impl<Key, Value, Hasher, KeyEqual>, param_hash_map<Key, Value>>
	{
	public:
		param_hash_map_impl()
		{
		}

		param_hash_map_impl(std::initializer_list<std::pair<const Key, Value>> list) : buffer_(std::move(list))
		{
		}

		template<typename OtherHasher, typename OtherKeyEqual, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<OtherHasher, Hasher>, std::is_convertible<OtherKeyEqual, KeyEqual>>>>
		param_hash_map_impl(OtherHasher&& hasher, OtherKeyEqual&& key_equal) : buffer_(0, std::forward<OtherHasher>(hasher), std::forward<OtherKeyEqual>(key_equal))
		{
		}

		template<typename OtherHasher, typename OtherKeyEqual, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<OtherHasher, Hasher>, std::is_convertible<OtherKeyEqual, KeyEqual>>>>
		param_hash_map_impl(std::initializer_list<std::pair<const Key, Value>> list, OtherHasher&& hasher, OtherKeyEqual&& key_equal) : buffer_(std::move(list), 0, std::forward<OtherHasher>(hasher), std::forward<OtherKeyEqual>(key_equal))
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

		Value get_value(const Key& key) const
		{
			auto iter = buffer_.find(key);
			
			if constexpr (impl::has_to_param_string_v<Key>)
			{
				return iter != buffer_.end() ? iter->second : throw abi_key_not_found{ format(FMT_STRING(u8"Key = {}"), to_param_string(key)) };
			}
			else if constexpr (std::is_same_v<Key, param_string>)
			{
				return iter != buffer_.end() ? iter->second : throw abi_key_not_found{ format(FMT_STRING(u8"Key = {}"), key) };
			}
			else
			{
				return iter != buffer_.end() ? iter->second : throw abi_key_not_found{};
			}
		}

		bool try_get_value(const Key& key, Value& value) const
		{
			auto iter = buffer_.find(key);

			return iter != buffer_.end() ? (value = iter->second, true) : false;
		}

		void add_or_update(const Key& key, const Value& value)
		{
			buffer_.insert_or_assign(key, value);
		}

		bool contains(const Key& key) const
		{
			return buffer_.find(key) != buffer_.end();
		}

		void remove(const Key& key)
		{
			if (auto iter = buffer_.find(key); iter != buffer_.end())
			{
				buffer_.erase(iter);
			}
		}

		void clear()
		{
			buffer_.clear();
		}

		object_iterator<param_pair<Key, Value>> get_iterator() const
		{
			return make_as_first<object_iterator_impl>(*this);
		}
	private:
		/// <summary>
		/// Implements an object iterator.
		/// </summary>
		class object_iterator_impl : public implements<object_iterator_impl, object_iterator<param_pair<Key, Value>>>
		{
		public:
			object_iterator_impl(const param_hash_map_impl& impl) : iter_{ impl.buffer_.begin() }, iter_end_{ impl.buffer_.end() }
			{
			}

			param_pair<Key, Value> current() const
			{
				return make_param_pair(iter_->first, iter_->second);
			}

			bool valid() const
			{
				return iter_ != iter_end_;
			}

			bool move_to_next()
			{
				return ++iter_ != iter_end_;
			}
		private:
			typename std::unordered_map<Key, Value>::const_iterator iter_;
			typename std::unordered_map<Key, Value>::const_iterator iter_end_;
		};

		std::unordered_map<Key, Value, Hasher, KeyEqual> buffer_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Creates a hash map.
	/// </summary>
	/// <typeparam name="Key">The key type</typeparam>
	/// <typeparam name="Value">The value type</typeparam>
	/// <returns>The hash map</returns>
	template<typename Key, typename Value, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<Key>, impl::has_abi_type<Value>>>>
	auto make_param_hash_map()
	{
		return make_as_first<impl::param_hash_map_impl<Key, Value>>();
	}

	/// <summary>
	/// Creates a hash map.
	/// </summary>
	/// <typeparam name="Key">The key type</typeparam>
	/// <typeparam name="Value">The value type</typeparam>
	/// <param name="list">The initializer list</param>
	/// <returns>The hash map</returns>
	template<typename Key, typename Value, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<Key>, impl::has_abi_type<Value>>>>
	auto make_param_hash_map(std::initializer_list<std::pair<const Key, Value>> list)
	{
		return make_as_first<impl::param_hash_map_impl<Key, Value>>(list);
	}

	/// <summary>
	/// Creates a hash map.
	/// </summary>
	/// <typeparam name="Key">The key type</typeparam>
	/// <typeparam name="Value">The value type</typeparam>
	/// <typeparam name="Hasher">The hasher type</typeparam>
	/// <typeparam name="KeyEqual">The equal type</typeparam>
	/// <returns>The hash map</returns>
	template<typename Key, typename Value, typename Hasher, typename KeyEqual, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<Key>, impl::has_abi_type<Value>>>>
	auto make_param_hash_map(Hasher&& hasher, KeyEqual&& key_equal)
	{
		return make_as_first<impl::param_hash_map_impl<Key, Value, std::decay_t<Hasher>, std::decay_t<KeyEqual>>>(std::forward<Hasher>(hasher), std::forward<KeyEqual>(key_equal));
	}

	/// <summary>
	/// Creates a hash map.
	/// </summary>
	/// <typeparam name="Key">The key type</typeparam>
	/// <typeparam name="Value">The value type</typeparam>
	/// <typeparam name="Hasher">The hasher type</typeparam>
	/// <typeparam name="KeyEqual">The equal type</typeparam>
	/// <param name="list">The initializer list</param>
	/// <returns>The hash map</returns>
	template<typename Key, typename Value, typename Hasher, typename KeyEqual, typename = std::enable_if_t<std::conjunction_v<impl::has_abi_type<Key>, impl::has_abi_type<Value>>>>
	auto make_param_hash_map(std::initializer_list<std::pair<const Key, Value>> list, Hasher&& hasher, KeyEqual&& key_equal)
	{
		return make_as_first<impl::param_hash_map_impl<Key, Value, std::decay_t<Hasher>, std::decay_t<KeyEqual>>>(list, std::forward<Hasher>(hasher), std::forward<KeyEqual>(key_equal));
	}
}

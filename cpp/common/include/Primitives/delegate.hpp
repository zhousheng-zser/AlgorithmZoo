#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <type_traits>

namespace glasssix
{
	class delegate_token
	{
	public:
		delegate_token() : id_{}
		{
		}

		delegate_token(const std::function<void(std::uint64_t)>& remove_handler, std::uint64_t id, const std::weak_ptr<void>& observer) : id_{ id }, observer_{ observer }, remove_handler_{ remove_handler }
		{
		}

		delegate_token(const delegate_token& other) = delete;

		delegate_token(delegate_token&& other) noexcept : id_{ std::exchange(other.id_, 0) }, observer_{ std::move(other.observer_) }, remove_handler_{ std::move(other.remove_handler_) }
		{
		}

		~delegate_token()
		{
			clear();
		}

		delegate_token& operator=(const delegate_token& right) = delete;
		
		delegate_token& operator=(delegate_token&& right) noexcept
		{
			clear();
			id_ = std::exchange(right.id_, 0);
			observer_ = std::move(right.observer_);
			remove_handler_ = std::move(right.remove_handler_);

			return *this;
		}

		void clear()
		{
			// We assume that the object keeps alive with the observer.
			if (auto observer = observer_.lock(); observer && remove_handler_)
			{
				remove_handler_(id_);
			}
		}
	private:
		std::uint64_t id_;
		std::weak_ptr<void> observer_;
		std::function<void(std::uint64_t)> remove_handler_;
	};

	template<typename Result, typename... Args>
	class delegate
	{
	public:
		using function_type = std::function<Result(Args...)>;

		delegate() : lifetime_observer_{ std::make_shared<int>() }
		{
			update_readable_buffer();
		}

		decltype(auto) operator+=(const delegate& right)
		{
			for (auto& [id, func] : right.listeners_)
			{
				add_listener(func);
			}

			return (update_readable_buffer(), *this);
		}

		template<typename Callable, typename = std::enable_if_t<std::is_convertible_v<Callable, function_type>>>
		decltype(auto) operator+=(Callable&& handler)
		{
			return (add_listener_with_updating(std::forward<Callable>(handler)), *this);
		}

		template<typename... Equivalents, typename = std::void_t<decltype(std::declval<function_type>()(std::declval<Equivalents>()...))>>
		Result operator()(Equivalents&&... args)
		{
			// Fetches the buffer automically.
			std::shared_ptr<function_type> buffer = std::atomic_load_explicit(&readable_buffer_, std::memory_order_acquire);

			if constexpr (std::is_void_v<Result>)
			{
				for (auto ptr = buffer.get(); *ptr; ptr++)
				{
					(*ptr)(std::forward<Equivalents>(args)...);
				}
			}
			else
			{
				// Initializes the default value.
				Result result{};

				for (auto ptr = buffer.get(); *ptr; ptr++)
				{
					result = (*ptr)(std::forward<Equivalents>(args)...);
				}

				return result;
			}
		}

		template<typename Callable, typename = std::enable_if_t<std::is_convertible_v<Callable, function_type>>>
		auto add_listener_auto_removal(Callable&& handler)
		{
			auto id = add_listener_with_updating(std::forward<Callable>(handler));

			return std::make_shared<delegate_token>(std::bind(&delegate::remove_listener, this, std::placeholders::_1), id, lifetime_observer_);
		}

		void remove_listener(std::uint64_t id)
		{
			if (auto iter = std::find_if(listeners_.begin(), listeners_.end(), [&](const auto& inner) { return inner.first == id; }); iter != listeners_.end())
			{
				// Creates a new readable buffer atomically.
				listeners_.erase(iter);
				update_readable_buffer();
			}
		}
	private:
		template<typename Callable>
		auto add_listener_with_updating(Callable&& handler)
		{
			std::uint64_t id = add_listener(std::forward<Callable>(handler));

			return (update_readable_buffer(), id);
		}

		template<typename Callable>
		auto add_listener(Callable&& handler)
		{
			return listeners_.emplace_back(global_counter_++, std::forward<Callable>(handler)).first;
		}

		void update_readable_buffer()
		{
			std::shared_ptr<function_type> buffer{ new function_type[listeners_.size() + 1], [](function_type* inner) { delete[] inner; } };

			for (std::size_t i = 0; i < listeners_.size(); i++)
			{
				buffer.get()[i] = listeners_[i].second;
			}

			std::atomic_store_explicit(&readable_buffer_, buffer, std::memory_order_release);
		}

		std::shared_ptr<void> lifetime_observer_;
		std::shared_ptr<function_type> readable_buffer_;
		std::vector<std::pair<std::uint64_t, function_type>> listeners_;
		inline static std::atomic_uint64_t global_counter_ = 0;
	};
}

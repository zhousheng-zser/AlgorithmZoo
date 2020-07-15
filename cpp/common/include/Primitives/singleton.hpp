#pragma once

#include <mutex>
#include <memory>
#include <utility>
#include <cstdint>
#include <type_traits>

namespace glasssix
{
	/// <summary>
	/// A singleton pattern.
	/// </summary>
	template<typename Object, typename = std::enable_if_t<std::is_class<Object>::value>>
	class singleton
	{
	public:
		virtual ~singleton() = default;

		template<typename... Args>
		static Object& instance(Args&&... args)
		{
			static std::once_flag flag;
			static std::aligned_storage_t<sizeof(Object), alignof(Object)> buffer;
			static std::shared_ptr<Object> result;

			std::call_once(flag, [&] { result.reset(::new (&buffer) Object{ std::forward<Args>(args)... }, [](Object* inner) { inner->~Object(); }); });
			
			return *result;
		}
	protected:
		singleton() = default;
	};

		
	/// <summary>
	/// An init-once resource initializer.
	/// </summary>
	template<typename T>
	class init_once : public singleton<T>
	{
	public:
		void invoke()
		{
			if (!has_initialized_)
			{
				init_environment_core();
				has_initialized_ = true;
			}
		}

		virtual ~init_once() = default;
	protected:
		virtual void init_environment_core() = 0;
	private:
		bool has_initialized_ = false;
	};
}
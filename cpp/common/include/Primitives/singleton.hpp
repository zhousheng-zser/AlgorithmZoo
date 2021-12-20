#pragma once

#include <utility>
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
			static Object result{ std::forward<Args>(args)... };
			
			return result;
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
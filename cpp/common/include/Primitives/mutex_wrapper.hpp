#ifndef _MUTEX_WRAPPER_
#define _MUTEX_WRAPPER_

#if defined(_MSC_VER)

#include <memory>

#include <Windows.h>

namespace glasssix
{
	/// <summary>
	/// A guard wrapper around the Windows Critical Section.
	/// </summary>
	class mutex_guard
	{
	public:
		mutex_guard(const std::reference_wrapper<CRITICAL_SECTION>& section) : section_{ section }
		{
			EnterCriticalSection(&section_.get());
		}

		~mutex_guard()
		{
			LeaveCriticalSection(&section_.get());
		}
	private:
		std::reference_wrapper<CRITICAL_SECTION> section_;
	};

	/// <summary>
	/// A wrapper around the Windows Critical Section.
	/// </summary>
	class mutex_wrapper
	{
	public:
		mutex_wrapper()
		{
			InitializeCriticalSection(&section_);
		}

		~mutex_wrapper()
		{
			DeleteCriticalSection(&section_);
		}

		inline mutex_guard guard()
		{
			return mutex_guard{ std::ref(section_) };
		}
	private:
		CRITICAL_SECTION section_;
	};
}

#elif defined(__linux__)
#include <pthread.h>

namespace glasssix
{
	class CRITICAL_SECTION
	{
	public:
		CRITICAL_SECTION()
		{
			pthread_mutex_init(&mtx, NULL);
		}

		~CRITICAL_SECTION()
		{
			pthread_mutex_destroy(&mtx);
		}

		void lock()
		{
			pthread_mutex_lock(&mtx);
		}

		void unlock()
		{
			pthread_mutex_unlock(&mtx);
		}

	private:
		pthread_mutex_t mtx;
	};

	class mutex_guard
	{
	public:
		mutex_guard(const std::reference_wrapper<CRITICAL_SECTION>& section) : section_{ section }
		{
			section_.get().lock();
		}

		~mutex_guard()
		{
			section_.get().unlock();
		}
	private:
		std::reference_wrapper<CRITICAL_SECTION> section_;
	};

	class mutex_wrapper
	{
	public:
		mutex_wrapper()
		{
		}

		~mutex_wrapper()
		{
		}

		inline mutex_guard guard()
		{
			return mutex_guard{ std::ref(section_) };
		}
	private:
		CRITICAL_SECTION section_;
	};
}

#endif

#endif
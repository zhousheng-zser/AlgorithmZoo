#pragma once

#ifdef __has_include
#if __has_include(<memory_resource>)
#include <memory_resource>
namespace hide_exp = std;
#elif __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
#include <experimental/vector>
namespace hide_exp = std::experimental;
#else
#error "Failed to find available <memory_resource> header. Please upgrade your C++ compiler."
#endif
#else
#error "Please compile this file with C++17 on."
#endif

#include <memory>

namespace glasssix::irisviel
{
	std::shared_ptr<hide_exp::pmr::memory_resource> make_synchronized_pool_resource_workaround() noexcept;

	/// <summary>
	/// An alternative to libc++ that does not implement concrete subclasses of memory_resource.
	/// </summary>
	class g6_synchronized_pool_resource : public hide_exp::pmr::memory_resource
	{
	public:
		virtual ~g6_synchronized_pool_resource() = default;
	protected:
		virtual void* do_allocate(size_t bytes, size_t align) override;
		virtual void do_deallocate(void* ptr, size_t bytes, size_t align) override;
		virtual bool do_is_equal(const hide_exp::pmr::memory_resource& that) const noexcept override;
	};
}

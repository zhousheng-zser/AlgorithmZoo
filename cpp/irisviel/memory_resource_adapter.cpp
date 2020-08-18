#include "memory_resource_adapter.hpp"
#include "Primitives/abi/meta.hpp"

#include <type_traits>

namespace glasssix::irisviel
{
	void* g6_synchronized_pool_resource::do_allocate(size_t bytes, size_t align)
	{
		return ::operator new(bytes, static_cast<std::align_val_t>(align));
	}

	void g6_synchronized_pool_resource::do_deallocate(void* ptr, size_t bytes, size_t align)
	{
		::operator delete(ptr, bytes, static_cast<std::align_val_t>(align));
	}

	bool g6_synchronized_pool_resource::do_is_equal(const hide_exp::pmr::memory_resource& that) const noexcept
	{
		return dynamic_cast<const g6_synchronized_pool_resource*>(&that);
	}

	std::shared_ptr<hide_exp::pmr::memory_resource> make_synchronized_pool_resource_workaround() noexcept
	{
#ifdef __GNUC__
		return std::make_shared<g6_synchronized_pool_resource>();
#else
		return std::make_shared<hide_exp::pmr::synchronized_pool_resource>();
#endif
	}
}

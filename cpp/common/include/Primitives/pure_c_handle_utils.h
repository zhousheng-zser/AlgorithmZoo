#pragma once

#ifdef __cplusplus

#include <utility>
#include <type_traits>

extern "C"
{
#endif

#define DEFINE_PURE_C_HANDLE(name) typedef /*struct tag_##name##_handle { void* opaque; }*/void* name##_handle

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace glasssix
{
	namespace pure_c
	{
		template<typename Handle, typename T>
		constexpr Handle to_handle(T* obj)
		{
			return Handle{ obj };
		}

		template<typename T, typename Handle>
		constexpr auto from_handle(Handle handle)
		{
			return static_cast<T*>(handle);
		}
	}
}
#endif

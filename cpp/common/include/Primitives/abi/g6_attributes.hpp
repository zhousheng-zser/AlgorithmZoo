#pragma once

#ifdef _WIN32
#define G6_ABI_CALL __stdcall
#else
#define G6_ABI_CALL
#endif

#if defined(__has_cpp_attribute) &&  __has_cpp_attribute(no_unique_address)
#define G6_EBO [[no_unique_address]]
#elif defined(_MSC_VER)
#define G6_EBO __declspec(empty_bases)
#else
#define G6_EBO
#endif

#ifdef _MSC_VER
#define G6_NOVTABLE __declspec(novtable)
#else
#define G6_NOVTABLE
#endif

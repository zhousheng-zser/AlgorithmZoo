#pragma once

#ifdef _WIN32
#ifdef _WIN64
#define GLASSSIX_ENVIRONMENT_64_BITS
#else
#define GLASSSIX_ENVIRONMENT_32_BITS
#endif
#elif defined(__linux__)
#if defined(__ppc64__) || defined(__x86_64__) || defined(__aarch64__)
#define GLASSSIX_ENVIRONMENT_64_BITS
#else
#define GLASSSIX_ENVIRONMENT_32_BITS
#endif
#else
#error "Failed to detect the manufacturer of the compiler."
#endif

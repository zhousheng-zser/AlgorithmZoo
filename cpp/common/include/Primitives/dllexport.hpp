#pragma once

#ifdef EXPORT_EXCALIBUR_PRIMITIVES
#undef EXPORT_EXCALIBUR_PRIMITIVES
#ifdef _MSC_VER // For Windows
#ifdef _WINDLL // Dynamic lib
#define EXPORT_EXCALIBUR_PRIMITIVES __declspec(dllexport)
#else // Static lib
#define EXPORT_EXCALIBUR_PRIMITIVES
#endif // !_WINDLL
#elif defined(__linux__) // For Linux
#define EXPORT_EXCALIBUR_PRIMITIVES __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define EXPORT_EXCALIBUR_PRIMITIVES __declspec(dllimport)
#elif defined(__linux__)
#define EXPORT_EXCALIBUR_PRIMITIVES __attribute__((visibility("default")))
#endif
#endif

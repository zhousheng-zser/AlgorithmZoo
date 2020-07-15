#pragma once

#ifdef __ANDROID__
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#elif defined(__has_include) && __has_include(<filesystem>)
#include <filesystem>
#if __cpp_lib_filesystem >= 201703L
namespace fs = std::filesystem;
#else
#error "Incorrect version of <filesystem>, please check your configurations."
#endif
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

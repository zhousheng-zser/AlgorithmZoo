cmake_minimum_required(VERSION 3.14.3)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	message(WARNING "CMAKE_TOOLCHAIN_FILE: please specify cmkae toolchain file in cmake command line.")
endif()
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

# For Visual Studio Generator (not Ninja), it is unneccessary to set CMAKE_MAKE_PROGRAM.
if (NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")
	if(NOT DEFINED CMAKE_MAKE_PROGRAM)
		message(WARNING "CMAKE_MAKE_PROGRAM: please specify make program in cmake command line.")
	endif()

	message(STATUS "CMAKE_MAKE_PROGRAM: ${CMAKE_MAKE_PROGRAM}")
endif()

project(AlgorithmZoo C CXX)

option(BUILD_UNIT_TEST "Build unit test." OFF)
option(COSINE_DISTANCE "Use cosine distance in irsiviel." ON)
option(USE_OPENMP "Use OpenMP" OFF)
option(HAVE_SSE "Control CPU SSE options when using gcc or clang" OFF)
option(HAVE_AVX "Control CPU AVX options when using gcc or clang" OFF)
option(HAVE_AVX2 "Control CPU AVX2 options when using gcc or clang" OFF)
option(USE_OPENCV "Use OpenCV library." ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
endif()

#set(Boost_INCLUDE_DIRS C:/Tools/vcpkg/installed/x64-windows/include)

set(COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/common/include ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/common/include/Primitives)

if(NOT DEFINED COMMON_LIBRARY_DIRS)
	message(FATAL_ERROR "COMMON_LIBRARY_DIRS NOT DEFINED: You must specify common library's directories.")
endif()

list(APPEND COMMON_LIBRARIES_NAMES primitives julius excalibur)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	foreach(library_name ${COMMON_LIBRARIES_NAMES})
		find_library(library
			NAMES ${library_name}
			PATHS ${COMMON_LIBRARY_DIRS}
			NO_DEFAULT_PATH
		)
		message(STATUS "${library_name} ${library}")
		list(APPEND COMMON_LIBRARIES ${library})
		unset(library CACHE)
	endforeach()
else()
	set(COMMON_LIBRARIES ${COMMON_LIBRARIES_NAMES})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	message(STATUS "ANDROID_NDK: ${ANDROID_NDK}")
	message(STATUS "ANDROID_ABI: ${ANDROID_ABI}")
	message(STATUS "ANDROID_PLATFORM: ${ANDROID_PLATFORM}")
	message(STATUS "ANDROID_TOOLCHAIN: ${ANDROID_TOOLCHAIN}")
	message(STATUS "ANDROID_STL: ${ANDROID_STL}")
endif()

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	if(HAVE_AVX2)
		add_compile_options(-mavx2)
		add_definitions(-D__AVX2__)
	endif()
	if(HAVE_AVX)
		add_compile_options(-mavx -mfma)
		add_definitions(-D__AVX__ -D__FMA__)
	endif()
	if(HAVE_SSE)
		add_compile_options(-mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mf16c)
		add_definitions(-D__SSE4_2__ -D__SSE4_1__ -D__SSSE3__ -D__SSE3__)
	endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	add_compile_options(-frtti -fexceptions)
endif()

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	add_compile_options(-fPIC -Wall -fpermissive)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_IMPORT_LIBRARY_PREFIX lib)
	set(CMAKE_SHARED_LIBRARY_PREFIX lib)
	set(CMAKE_STATIC_LIBRARY_PREFIX lib)
endif()

include(Dependencies.cmake)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_definitions(-DFMT_SHARED)

add_subdirectory(hardcode)
add_subdirectory(Cassius)
add_subdirectory(Gaius)
add_subdirectory(Longinus)
add_subdirectory(Irisviel)
add_subdirectory(Romancia)
if(BUILD_UNIT_TEST)
	add_subdirectory(UnitTest)
endif()
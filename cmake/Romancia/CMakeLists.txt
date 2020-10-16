cmake_minimum_required(VERSION 3.14.3)
set(module_name romancia)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/romancia/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/romancia/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_definitions(${module_name} PRIVATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})

if(USE_OPENCV)
	target_include_directories(${module_name} PRIVATE ${OPENCV_INCLUDE_DIRS})
	target_link_directories(${module_name} PRIVATE ${OPENCV_LIBRARY_DIRS})
	target_link_libraries(${module_name} PRIVATE opencv_java3)
endif()
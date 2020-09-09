cmake_minimum_required(VERSION 3.14.3)
set(module_name longinus)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/longinus/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/longinus/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})
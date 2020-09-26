cmake_minimum_required(VERSION 3.14.3)
set(target_name hardcode)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode/*.hpp)

add_library(${target_name} ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${target_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

target_include_directories(${target_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${target_name} PRIVATE ${COMMON_LIBRARIES})

cmake_minimum_required(VERSION 3.14.3)
set(module_name irisviel)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/irisviel/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/irisviel/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

if(COSINE_DISTANCE)
	target_compile_definitions(${module_name} PRIVATE -DCOSINE_DISTANCE)
endif()

if(USE_OPENMP)
	target_link_libraries(${module_name} OpenMP::OpenMP_CXX)
endif()

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_link_libraries(${module_name} PRIVATE bcrypt)
endif()

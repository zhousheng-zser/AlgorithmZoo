cmake_minimum_required(VERSION 3.14.3)

#add_library(pan SHARED "pan.cpp" "pan.h")

set(module_name pan)
set(hardcode_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pan/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pan/*.hpp)


################ Eigen3 ###################
#if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
#	list(APPEND CMAKE_PREFIX_PATH "D:/lib/eigen-3.4.0/out/build/x64-Debug")
#elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
#	list(APPEND CMAKE_PREFIX_PATH "D:/lib/eigen-3.4.0/out/build/x64-Release")
#else()
#    message("else:${CMAKE_BUILD_TYPE}")
#    message("else:${CMAKE_C_FLAGS_RELEASE}")
#    add_executable(test_release ${src_dirs})
#endif()
#find_package(Eigen3 REQUIRED)
#if(Eigen3_FOUND)
#else()
#	message("Eigen3 not found")
#	set(EIGEN3_INCLUDE_DIR  "D:/lib/eigen-3.4.0")
#endif()

#set(EIGEN3_INCLUDE_DIR  "D:/lib/eigen-3.4.0")
set(EIGEN3_INCLUDE_DIR  "D:/vcpkg-master/installed/x64-windows/include/eigen3")
#INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

add_library(${module_name} SHARED ${src} ${header}) 

target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS} ${OpenCV_LIBRARY_DIRS})
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} ${hardcode_dir} ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} hardcode ${OpenCV_LIBS})
add_dependencies(${module_name} hardcode)

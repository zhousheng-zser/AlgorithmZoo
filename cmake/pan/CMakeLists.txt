cmake_minimum_required(VERSION 3.14.3)

set(module_name pan)
set(hardcode_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pan/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pan/*.hpp)

set(EIGEN3_INCLUDE_DIR  "D:/vcpkg/installed/x64-windows/include/eigen3")

add_library(${module_name} SHARED ${src} ${header}) 

target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS} ${OpenCV_LIBRARY_DIRS})
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} ${hardcode_dir} ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} hardcode ${OpenCV_LIBS})
add_dependencies(${module_name} hardcode)

cmake_minimum_required(VERSION 3.14.3)

set(module_name plate)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/plate/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/plate/*.hpp)
add_library(${module_name} SHARED ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})

target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
if(NOT OPENCV_FOUND)
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()
target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})
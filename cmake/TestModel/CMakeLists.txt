cmake_minimum_required(VERSION 3.5)

set(module_name test_model)

file(GLOB CPP_FILES ${PROJECT_SOURCE_DIR}/../cpp/test_model/*.cpp)
file(GLOB HPP_FILES ${PROJECT_SOURCE_DIR}/../cpp/test_model/*.hpp)

add_executable(${module_name} ${CPP_FILES} ${HPP_FILES})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
if(NOT OPENCV_FOUND)
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()
target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})
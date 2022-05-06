cmake_minimum_required(VERSION 3.14.3)

set(module_name yolov5)
set(hardcode_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/yolov5/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/yolov5/*.hpp)

add_library(${module_name} SHARED ${src} ${header})


if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} ${hardcode_dir} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} hardcode ${OpenCV_LIBS})

add_dependencies(${module_name} hardcode)
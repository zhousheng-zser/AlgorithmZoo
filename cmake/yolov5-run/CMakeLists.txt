cmake_minimum_required(VERSION 3.14.3)

set(module_name "yolov5-run")

file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/yolov5-run/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/yolov5-run/*.hpp)

add_executable(${module_name} ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()

target_include_directories(${module_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp)
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})

target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})




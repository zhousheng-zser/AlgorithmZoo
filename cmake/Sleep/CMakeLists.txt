cmake_minimum_required(VERSION 3.14.3)
set(module_name sleep)
set(hardcode_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/hardcode)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/sleep/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/sleep/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
if(${RKNNAPI_VERSION})
	target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
	if(NOT OPENCV_FOUND)
		target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
	endif()
	target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})
endif()

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} ${hardcode_dir})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} hardcode)

add_dependencies(${module_name} hardcode)
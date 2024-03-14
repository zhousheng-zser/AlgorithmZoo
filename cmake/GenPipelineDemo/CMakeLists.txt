cmake_minimum_required(VERSION 3.5)

set(module_name genpipeline_demo)
cmake_minimum_required(VERSION 3.5)

file(GLOB SRC 
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline_demo/*.hpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline_demo/*.cpp
)

add_executable(${module_name} ${SRC}) 

target_link_libraries(${module_name} PRIVATE genpipeline) #cant miss PRIVATE, must be either all-keyword or all-plain.
add_dependencies(${module_name} genpipeline)


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

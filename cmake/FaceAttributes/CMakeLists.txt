cmake_minimum_required(VERSION 3.14.3)
set(module_name face_attributes)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/faceattributes/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/faceattributes/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/faceattributes/*.h)
add_library(${module_name} SHARED ${src} ${header})
target_link_libraries(${module_name} PRIVATE genpipeline)
add_dependencies(${module_name} genpipeline)

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_definitions(${module_name} PRIVATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})

target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
if(NOT OPENCV_FOUND)
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()
target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})


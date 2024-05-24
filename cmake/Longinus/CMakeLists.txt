cmake_minimum_required(VERSION 3.14.3)
set(module_name longinus)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/longinus/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/longinus/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

target_link_libraries(${module_name} PRIVATE genpipeline)
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

target_include_directories(${module_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/3rdparty/jsoncpp/include)
target_link_libraries(${module_name} PRIVATE jsoncpp)
add_dependencies(${module_name} jsoncpp)


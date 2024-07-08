cmake_minimum_required(VERSION 3.14.3)
set(module_name pump_work_status)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pump_work_status/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/pump_work_status/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

target_include_directories(${module_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/3rdparty/jsoncpp/include)
target_link_libraries(${module_name} PRIVATE jsoncpp)
add_dependencies(${module_name} jsoncpp)

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

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} )
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} )


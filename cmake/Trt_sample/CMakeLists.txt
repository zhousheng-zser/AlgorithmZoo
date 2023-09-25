cmake_minimum_required(VERSION 3.14.3)
set(module_name trt_sample)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/trt_sample/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/trt_sample/*.hpp)

# add_library(${module_name} SHARED ${src} ${header})
add_executable(${module_name} ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

if(${USE_TENSORRT})
	target_include_directories(${module_name} PUBLIC ${CUDA_INCLUDE_DIRS})
	target_link_directories(${module_name} PUBLIC ${CUDA_LIBRARY_DIRS})
	target_link_libraries(${module_name} PUBLIC ${CUDA_LIBRARIES})

	target_include_directories(${module_name} PUBLIC ${TENSORRT_INCLUDE_DIRS})
	target_link_directories(${module_name} PUBLIC ${TENSORRT_LIBRARY_DIRS})
	target_link_libraries(${module_name} PUBLIC ${TENSORRT_LIBRARIES})
endif()

message(STATUS "COMMON_INCLUDE_DIRS:${COMMON_INCLUDE_DIRS}")

target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} )
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES} )

target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})
cmake_minimum_required(VERSION 3.14.3)
set(module_name onphone)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/onphone/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/onphone/*.hpp)

add_library(${module_name} SHARED ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
endif()

target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
if(NOT OPENCV_FOUND)
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()
target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})



target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS} )
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})


#option(USE_ONNXRT "USE ONNXRT" ON)
#if(USE_ONNXRT)
#add_definitions(-DUSE_ONNXRT)
# onnxruntime
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(ONNXRT_DIR "E:/vcpkg/packages/onnxruntime-gpu_x64-windows")
	message("####### target_include_directories : ${ONNXRT_DIR}/include")
	target_include_directories(${module_name} PRIVATE ${ONNXRT_DIR}/include)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_link_directories(${module_name} PRIVATE ${ONNXRT_DIR}/lib)
	else()
		target_link_directories(${module_name} PRIVATE ${ONNXRT_DIR}/debug/lib)
	endif()
	target_link_libraries(${module_name} PRIVATE onnxruntime)
endif()
#endif()
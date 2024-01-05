cmake_minimum_required(VERSION 3.5)

set(module_name test_model)

file(GLOB CPP_FILES ${PROJECT_SOURCE_DIR}/../cpp/test_model/*.cpp ${PROJECT_SOURCE_DIR}/../cpp/test_model/postprocessing_functions/*.cpp)
file(GLOB HPP_FILES ${PROJECT_SOURCE_DIR}/../cpp/test_model/*.hpp)

file(GLOB argparse_files ${PROJECT_SOURCE_DIR}/../cpp/test_model/argparse/*.hpp ${PROJECT_SOURCE_DIR}/../cpp/test_model/argparse/*.cpp)
file(GLOB numpy_extensor_files ${PROJECT_SOURCE_DIR}/../cpp/test_model/numpy_extensor/*.hpp ${PROJECT_SOURCE_DIR}/../cpp/test_model/numpy_extensor/*.cpp)

add_executable(${module_name} ${CPP_FILES} ${HPP_FILES} ${argparse_files} ${numpy_extensor_files}) 

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	message("Use onnxruntime, ONNXRT_DIR:${ONNXRT_DIR}")
	#set(ONNXRT_DIR "E:/vcpkg/packages/onnxruntime-gpu_x64-windows" CACHE STRING "")
	target_include_directories(${module_name} PRIVATE ${ONNXRT_DIR}/include)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_link_directories(${module_name} PRIVATE ${ONNXRT_DIR}/lib)
	else()
		target_link_directories(${module_name} PRIVATE ${ONNXRT_DIR}/debug/lib)
	endif()
	target_link_libraries(${module_name} PRIVATE onnxruntime)

	add_definitions(-DUSE_ONNXRT)
endif()
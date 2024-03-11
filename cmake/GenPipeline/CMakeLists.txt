cmake_minimum_required(VERSION 3.5)

set(module_name genpipeline)

file(GLOB GENPIPELINE_FILE
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/GenPipeline/*.cpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/GenPipeline/*.hpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/GenPipeline/Backends/*.cpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/GenPipeline/Backends/*.hpp
)

file(GLOB GENPIP_COMMON_HEAD ${PROJECT_SOURCE_DIR}/../cpp/common/include/GenPipeline/*.hpp)

file(GLOB POSTPROCESSING_FILE
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/postprocessing/*.cpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/postprocessing/*.hpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/market/*.cpp
	${PROJECT_SOURCE_DIR}/../cpp/genpipeline/market/*.hpp
)

add_library(genpipeline SHARED
	${GENPIP_COMMON_HEAD}
	${GENPIPELINE_FILE} 
	${POSTPROCESSING_FILE}
) 

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_definitions(genpipeline PRIVATE -DFMT_EXPORT -DEXPORT_EXCALIBUR_PRIMITIVES -D_WINDLL)
endif()


#### Link Libraries
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
    target_link_directories(genpipeline PRIVATE ${COMMON_LIBRARY_DIRS})
endif()
target_include_directories(genpipeline PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(genpipeline PRIVATE ${COMMON_LIBRARIES})

target_include_directories(genpipeline PRIVATE ${OpenCV_INCLUDE_DIRS})
if(NOT OPENCV_FOUND)
    target_link_directories(genpipeline PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()
target_link_libraries(genpipeline PRIVATE ${OpenCV_LIBS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	#set(ONNXRT_DIR "E:/vcpkg/packages/onnxruntime-gpu_x64-windows" CACHE STRING "")
	if(NOT ${ONNXRT_DIR} STREQUAL "")
		message("Use onnxruntime, ONNXRT_DIR:${ONNXRT_DIR}")
		target_include_directories(genpipeline PRIVATE ${ONNXRT_DIR}/include)
		if(CMAKE_BUILD_TYPE STREQUAL "Release")
			target_link_directories(genpipeline PRIVATE ${ONNXRT_DIR}/lib)
		else()
			target_link_directories(genpipeline PRIVATE ${ONNXRT_DIR}/debug/lib)
		endif()
		target_link_libraries(genpipeline PRIVATE onnxruntime)

		add_definitions(-DUSE_ONNXRT)
		add_definitions(-DONNXRT_CUDA)
	endif()
else()
	#set(ONNXRT_DIR "/home/glasssix/yhc/onxrtval/onnxruntime-linux-aarch64-1.14.1" CACHE STRING "")
	if(NOT ${ONNXRT_DIR} STREQUAL "")
		target_include_directories(genpipeline PRIVATE ${ONNXRT_DIR}/include)
		target_link_directories(genpipeline PRIVATE ${ONNXRT_DIR}/lib)
		target_link_libraries(genpipeline PRIVATE onnxruntime)
		add_definitions(-DUSE_ONNXRT)
		add_definitions(-DONNXRT_CPU)
	endif()
endif()
if((NOT DEFINED Boost_INCLUDE_DIRS) OR (Boost_INCLUDE_DIRS STREQUAL ""))
	find_package(Boost REQUIRED)
endif()

if(USE_CUDA)
	find_package(CUDA REQUIRED)

	if(${CUDA_VERSION} VERSION_LESS 11.1)
		message(FATAL_ERROR "CUDA version is too lower(${CUDA_VERSION} vs 11.1)")
	endif()

	set(CUDA_LIBRARY_DIRS ${CUDA_LIBRARIES})
	set(CUDA_LIBRARIES cuda cudart_static cublas)

	message(STATUS "CUDA_INCLUDE_DIRS = ${CUDA_INCLUDE_DIRS}")
	message(STATUS "CUDA_LIBRARY_DIRS = ${CUDA_LIBRARY_DIRS}")
	message(STATUS "CUDA_LIBRARY_DIRS = ${CUDA_LIBRARIES}")

	set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -allow-unsupported-compiler")

	if(USE_CUDNN)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			set(CUDNN_INCLUDE_DIRS ${CUDA_INCLUDE_DIRS})
			set(CUDNN_LIBRARY_DIRS ${CUDA_LIBRARIES})
		else(GNU OR Clang)
			set(CUDNN_INCLUDE_DIRS /usr/local/cuda/include)
			set(CUDNN_LIBRARY_DIRS /usr/local/cuda/lib64)
		endif()

		list(APPEND CUDNN_LIBRARIES cudnn)
		message(STATUS "CUDNN_INCLUDE_DIRS = ${CUDNN_INCLUDE_DIRS}")
		message(STATUS "CUDNN_LIBRARY_DIRS = ${CUDNN_LIBRARY_DIRS}")
	endif()
	if(USE_TENSORRT)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			# or only copy all the files into CUDA include & lib/x64
			set(TENSORRT_INCLUDE_DIRS ${TENSORRT_DIR}/include)
			set(TENSORRT_LIBRARY_DIRS ${TENSORRT_DIR}/lib)
		else(GNU OR Clang)
			set(TENSORRT_INCLUDE_DIRS /usr/local/cuda/include)
			set(TENSORRT_LIBRARY_DIRS /usr/local/cuda/lib64)
		endif()

		file(GLOB FILES "${TENSORRT_LIBRARY_DIRS}/nv*.lib")
		set(TENSORRT_LIBRARIES "")

		foreach(file ${FILES})
			get_filename_component(name ${file} NAME_WE)
			list(APPEND TENSORRT_LIBRARIES ${name})
		endforeach()

		message(STATUS "TENSORRT_INCLUDE_DIRS = ${TENSORRT_INCLUDE_DIRS}")
		message(STATUS "TENSORRT_LIBRARY_DIRS = ${TENSORRT_LIBRARY_DIRS}")
		message(STATUS "TENSORRT_LIBRARIES    = ${TENSORRT_LIBRARIES}")
	endif()
endif()

if(USE_OPENMP)
	find_package(OpenMP REQUIRED)
	add_compile_options(${OpenMP_CXX_FLAGS})
endif()

find_package(OpenCV 4.7)

if(NOT OPENCV_FOUND)
	message(WARNING "Not Found installed OpenCV, use mannual configuration.")

	if(NOT DEFINED OpenCV_INCLUDE_DIRS)
		message(FATAL_ERROR "Not define OpenCV_INCLUDE_DIRS")
	endif()

	if(NOT DEFINED OpenCV_LIBRARY_DIRS)
		message(FATAL_ERROR "Not define OpenCV_LIBRARY_DIRS")
	endif()

	if(NOT DEFINED OpenCV_LIBS)
		message(FATAL_ERROR "Not define OpenCV_LIBS")
	endif()

	#set(OpenCV_INCLUDE_DIRS E:/OpenCV-android-sdk/sdk/native/jni/include)
	#set(OpenCV_LIBRARY_DIRS E:/OpenCV-android-sdk/sdk/native/libs/${ANDROID_ABI})
	#set(OpenCV_LIBS opencv_java4)
endif()
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
if(NOT OPENCV_FOUND)
	message(STATUS "OpenCV_LIBRARY_DIRS: ${OpenCV_LIBRARY_DIRS}")
endif()
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")

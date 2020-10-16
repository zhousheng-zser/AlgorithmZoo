if((NOT DEFINED Boost_INCLUDE_DIRS) OR (Boost_INCLUDE_DIRS STREQUAL ""))
	find_package(Boost REQUIRED)
endif()

if(USE_OPENMP)
	find_package(OpenMP REQUIRED)
	add_compile_options(${OpenMP_CXX_FLAGS})
endif()

if(USE_OPENCV)
	set(OPENCV_INCLUDE_DIRS E:/OpenCV-android-sdk/sdk/native/jni/include)
	set(OPENCV_LIBRARY_DIRS E:/OpenCV-android-sdk/sdk/native/libs/arm64-v8a)
endif()
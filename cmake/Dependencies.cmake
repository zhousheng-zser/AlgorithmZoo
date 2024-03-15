if((NOT DEFINED Boost_INCLUDE_DIRS) OR (Boost_INCLUDE_DIRS STREQUAL ""))
	find_package(Boost REQUIRED)
endif()

if(USE_OPENMP)
	find_package(OpenMP REQUIRED)
	add_compile_options(${OpenMP_CXX_FLAGS})
endif()

if(NOT ${SOPHONE_SDK} STREQUAL "")
	set(OpenCV_INCLUDE_DIRS
		${SOPHONE_SDK}/include
		${SOPHONE_SDK}/include/opencv4)
	set(OpenCV_LIBRARY_DIRS ${SOPHONE_SDK}/lib)
	set(OpenCV_LIBS opencv_imgproc opencv_core opencv_highgui opencv_imgcodecs opencv_videoio)
else()
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
endif()
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
if(NOT OPENCV_FOUND)
	message(STATUS "OpenCV_LIBRARY_DIRS: ${OpenCV_LIBRARY_DIRS}")
endif()
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")

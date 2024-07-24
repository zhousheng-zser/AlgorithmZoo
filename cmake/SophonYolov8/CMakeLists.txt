cmake_minimum_required(VERSION 3.5)

# 设置模块名称
set(module_name yolov8_bmcv)

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/common/include/sophonyolov8/*.cpp)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/common/include/sophonyolov8/*.hpp)

# 设置库和可执行文件的输出路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../)

add_library(${module_name} SHARED ${SRC_FILES} ${HEADER_FILES})


if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_definitions(yolov8_bmcv PRIVATE -DFMT_EXPORT -DEXPORT_EXCALIBUR_PRIMITIVES -D_WINDLL)
endif()

# 设置库
set(BM_LIBS bmlib bmrt bmcv yuv)
set(JPU_LIBS bmjpuapi bmjpulite)
set(OPENCV_LIBS opencv_imgproc opencv_core opencv_highgui opencv_imgcodecs opencv_videoio)

target_link_directories(yolov8_bmcv PRIVATE ${COMMON_LIBRARY_DIRS})
target_include_directories(yolov8_bmcv PRIVATE ${COMMON_INCLUDE_DIRS})


# 包含头文件目录`
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/common/include)

# 链接库目录
link_directories(${OpenCV_LIBRARY_DIRS})
link_directories(/root/cw/Excalibur/cmake/sophon/lib)


# 链接库
target_link_libraries(yolov8_bmcv ${BM_LIBS} ${OPENCV_LIBS} ${JPU_LIBS} ${COMMON_LIBRARIES} -lpthread -lavcodec -lavformat -lavutil)
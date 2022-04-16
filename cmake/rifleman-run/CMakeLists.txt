#add_executable(rifleman-run "rifleman-run.cpp" "rifleman-run.h")

cmake_minimum_required(VERSION 3.14.3)

set(module_name "rifleman-run")

file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/rifleman-run/*.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/rifleman-run/*.hpp)

add_executable(${module_name} ${src} ${header})

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
	target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
	target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})
endif()

target_include_directories(${module_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp)
target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})
target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})

target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})
target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})


# copy dlls
set(OPENCV_INSTALL_DIR ${OpenCV_INCLUDE_DIRS}/..)
set(OPENCV_DLL_DIRS ${OPENCV_INSTALL_DIR}/x64/vc15/bin)
file(GLOB OPENCV_DLLS ${TORCH_LIBRARIES}/*.dll ${OPENCV_DLL_DIRS}/*.dll)
message(OPENCV_DLL_DIRS: ${OPENCV_DLL_DIRS})
message(OPENCV_DLLS: ${OPENCV_DLLS})


add_custom_command(TARGET ${module_name}  #目标项目
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENCV_DLLS}
        $<TARGET_FILE_DIR:rifleman-run>) #项目路径





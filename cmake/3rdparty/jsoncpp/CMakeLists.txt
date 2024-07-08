cmake_minimum_required(VERSION 3.14.3)
set(module_name jsoncpp)
file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/3rdparty/jsoncpp/src/jsoncpp.cpp)
file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/3rdparty/jsoncpp/include/jsoncpp.h)

add_library(${module_name} STATIC ${src} ${header})

target_include_directories(${module_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../cpp/3rdparty/jsoncpp/include)


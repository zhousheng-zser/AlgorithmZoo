#include "../Project.hpp"
#include <iostream>
#include <string>
#include "../string_process.hpp"

void noah::Projcet::CMakeLists_txt(const std::string& file_path) {
	FILE* fp = fopen((file_path + "CMakeLists.txt").c_str(), "wb");

	std::string module_name = this->module_name;

	TXTLines txtlines;
	txtlines.push("cmake_minimum_required(VERSION 3.14.3)");
	txtlines.push();
	txtlines.push("set(module_name ", module_name,')');
	txtlines.push("file(GLOB src ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/", module_name,"/*.cpp)");
	txtlines.push("file(GLOB header ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/", module_name,"/*.hpp)");
	txtlines.push("add_library(${module_name} SHARED ${src} ${header})");
	txtlines.push("");
	txtlines.push("if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL \"MSVC\"))");
	txtlines.push<1>("target_link_directories(${module_name} PRIVATE ${COMMON_LIBRARY_DIRS})");
	txtlines.push("endif()");
	txtlines.push("target_include_directories(${module_name} PRIVATE ${COMMON_INCLUDE_DIRS})");
	txtlines.push("target_link_libraries(${module_name} PRIVATE ${COMMON_LIBRARIES})");
	txtlines.push("#add_dependencies(${module_name} hardcode)");
	txtlines.push("");
	txtlines.push("target_include_directories(${module_name} PRIVATE ${OpenCV_INCLUDE_DIRS})");
	txtlines.push("if(NOT OPENCV_FOUND)");
	txtlines.push<1>("target_link_directories(${module_name} PRIVATE ${OpenCV_LIBRARY_DIRS})");
	txtlines.push("endif()");
	txtlines.push("target_link_libraries(${module_name} PRIVATE ${OpenCV_LIBS})");

	fprintf(fp, "%s", txtlines.export_string().c_str());
	fclose(fp);
}
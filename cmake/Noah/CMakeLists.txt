
cmake_minimum_required(VERSION 3.5)

set(sub_module noah)

file(GLOB UTEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/utest/*.hpp)
file(GLOB HEAD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/head/*.hpp)
file(GLOB CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/*.cpp)
file(GLOB TypeSysCPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/ABICLASSIR/*.cpp)
file(GLOB ExpFn_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/ProjectExports/*.cpp)
file(GLOB ExpFn_HEADS ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/ProjectExports/*.hpp)
file(GLOB HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/noah/*.hpp)

#target_include_directories(${module_name} PUBLIC ${Boost_INCLUDE_DIRS})

add_executable(${sub_module} ${CPP_FILES} ${TypeSysCPP_FILES} ${ExpFn_FILES} ${ExpFn_HEADS} ${HPP_FILES} ${HEAD_FILES} ${UTEST_FILES})


#message(">> ${PROJECT_SOURCE_DIR}")
#install(TARGETS ${sub_module} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
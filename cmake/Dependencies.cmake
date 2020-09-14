if((NOT DEFINED Boost_INCLUDE_DIRS) OR (Boost_INCLUDE_DIRS STREQUAL ""))
	find_package(Boost REQUIRED)
endif()

if(USE_OPENMP)
	find_package(OpenMP REQUIRED)
	add_compile_options(${OpenMP_CXX_FLAGS})
endif()
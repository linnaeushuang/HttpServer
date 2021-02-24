
cmake_minimum_required(VERSION 3.0)

project(test_utils)

set(LIB_SRC
	../AsyncLogging.cpp
	../CountDownLatch.cpp
	../FileUtils.cpp
	../LogFile.cpp
	../Logging.cpp
	../LogStream.cpp
	../Thread.cpp)


set( CMAKE_CXX_FLAGS "-std=c++11" )
add_library(libutils_base ${LIB_SRC})
target_link_libraries(libutils_base pthread rt)
set_target_properties(libutils_base PROPERTIES OUTPUT_NAME "utils_base")

add_executable(LogTest LogTest.cpp)
target_link_libraries(LogTest libutils_base)

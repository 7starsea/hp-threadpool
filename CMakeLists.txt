cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(example-hp-threadpool)

# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

if(UNIX)
	set(CMAKE_USE_PTHREADS_INIT "pthread")
endif(UNIX)

add_executable(example example.cpp)

target_link_libraries (example ${CMAKE_USE_PTHREADS_INIT})

cmake_minimum_required(VERSION 3.21)

project(OCLtest VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCL REQUIRED)

add_executable(OCLtest main.cpp ThreadPool.h)

if(MSVC)
  set_property(TARGET OCLtest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif(MSVC)
target_link_libraries(OCLtest PRIVATE OpenCL::OpenCL)

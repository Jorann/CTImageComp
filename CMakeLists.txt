cmake_minimum_required(VERSION 3.5)
project(imgc2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(imgc2 ${SOURCE_FILES})
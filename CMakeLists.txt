cmake_minimum_required(VERSION 3.6)
project(FileSystem)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES FileSystemManager.h FileSystemManager.cc Main.cc)
add_executable(FileSystem ${SOURCE_FILES})

# TODO: dodać wiecej flag
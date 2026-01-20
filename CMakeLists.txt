cmake_minimum_required(VERSION 3.6)
project(FileSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCE_FILES include/FileSystemManager.h src/FileSystemManager.cc src/Main.cc src/HelperMethods.cc)
add_executable(FileSystem ${SOURCE_FILES})


# --- Google Test ---
enable_testing()
add_subdirectory(libs/googletest EXCLUDE_FROM_ALL)

add_executable(FileSystemTests tests/FileSystemTests.cc src/FileSystemManager.cc src/HelperMethods.cc)
target_link_libraries(FileSystemTests gtest gtest_main)
target_include_directories(FileSystemTests PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_test(NAME FileSystemTests COMMAND FileSystemTests)
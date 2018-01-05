cmake_minimum_required(VERSION 3.8)

project(OS_coure_project)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
message(STATUS "GCC: Enabled c++17 support")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/lib)

include_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(lib)
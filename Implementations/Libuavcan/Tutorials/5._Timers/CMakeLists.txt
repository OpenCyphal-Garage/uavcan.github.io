cmake_minimum_required(VERSION 2.8)

project(tutorial_project)

find_library(UAVCAN_LIB uavcan REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")

# Make sure to provide correct path to 'platform_linux.cpp'! See earlier tutorials for more info.
add_executable(node node.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(node ${UAVCAN_LIB} rt)

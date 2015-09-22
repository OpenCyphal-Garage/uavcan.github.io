cmake_minimum_required(VERSION 2.8)

project(tutorial_project)

find_library(UAVCAN_LIB uavcan REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")

# Make sure to provide correct path to 'platform_linux.cpp'! See earlier tutorials for more info.
add_executable(server server.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(server ${UAVCAN_LIB} rt)

add_executable(client client.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(client ${UAVCAN_LIB} rt)

add_executable(client_cpp03 client_cpp03.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(client_cpp03 ${UAVCAN_LIB} rt)

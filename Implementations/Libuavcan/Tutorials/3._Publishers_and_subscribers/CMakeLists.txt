cmake_minimum_required(VERSION 2.8)

project(tutorial_project)

find_library(UAVCAN_LIB uavcan REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")

add_executable(publisher publisher.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(publisher ${UAVCAN_LIB} rt)

add_executable(subscriber subscriber.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(subscriber ${UAVCAN_LIB} rt)

add_executable(subscriber_cpp03 subscriber_cpp03.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(subscriber_cpp03 ${UAVCAN_LIB} rt)

cmake_minimum_required(VERSION 2.8)

project(tutorial_project)

find_library(UAVCAN_LIB uavcan REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11")

#
# For the next step, we're going to need to know where the standard DSDL definitions are located.
#
# For the sake of simplicity, this example assumes that the OS we're working on is Linux, so
# we can use the default installation directory for DSDL definitions in /usr/local/share/.
#
# If you're using it on an embedded system or if the library isn't installed in the host OS,
# use a relative path to the DSDL subproject.
#
set(UAVCAN_DSDL_DEFINITIONS "/usr/local/share/uavcan/dsdl/uavcan")      # For Linux, if the library is installed

# If the library is not installed on the host OS, use relative path to the standard DSDL definitions, for example:
#set(UAVCAN_DSDL_DEFINITIONS "dsdl/uavcan")

#
# Invoke the DSDL compiler to generate headers for our custom data types.
# The default output directory is "dsdlc_generated"; it can be overridden if needed.
# If libuavcan is installed, we can use directly the compiler's executable "libuavcan_dsdlc".
# If the library isn't installed, use a relative path to the compiler in the libuavcan source tree.
#
add_custom_target(dsdlc libuavcan_dsdlc                                      # If the library is installed
                  "./sirius_cybernetics_corporation" -I${UAVCAN_DSDL_DEFINITIONS}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Using relative path to the DSDL compiler if the library is not installed:
#add_custom_target(dsdlc libuavcan/libuavcan/dsdl_compiler/libuavcan_dsdlc
#                  "./sirius_cybernetics_corporation" -I${UAVCAN_DSDL_DEFINITIONS}
#                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include_directories("dsdlc_generated") # Adding the default output directory to the include paths

#
# Side note:
# A possible way to invoke the DSDL compiler using plain make is the following:
#       include libuavcan/libuavcan/include.mk
#       $(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR) "./sirius_cybernetics_corporation"))
# Assuming that the library is not installed.
# Don't forget to add "dsdlc_generated" to the include directories.
#

#
# Client and server targets.
# Note that both depend on the dsdlc target.
# Make sure to provide correct path to 'platform_linux.cpp'! See earlier tutorials for more info.
#
add_executable(server server.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(server ${UAVCAN_LIB} rt)
add_dependencies(server dsdlc)

add_executable(client client.cpp ${CMAKE_SOURCE_DIR}/../2._Node_initialization_and_startup/platform_linux.cpp)
target_link_libraries(client ${UAVCAN_LIB} rt)
add_dependencies(client dsdlc)

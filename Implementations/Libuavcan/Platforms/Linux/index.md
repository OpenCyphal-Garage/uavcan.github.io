---
---

# Libuavcan for Linux

This page describes the Libuavcan driver for Linux and shows how to use libuavcan on Linux.
The libuavcan driver for Linux is a header-only C++11 library that implements a fully functional platform interface
for libuavcan and also adds some convenient wrappers on top of libuavcan.
It's built on the following Linux components:

* [SocketCAN](http://en.wikipedia.org/wiki/SocketCAN) -
A generic CAN bus stack for Linux.
* [librt](http://www.lehman.cuny.edu/cgi-bin/man-cgi?librt+3) -
The POSIX.1b Realtime Extensions Library, formerly known as libposix4.
This is used for time measurements and time synchronization support.

Note that the driver will be able to synchronize the system clock with the network time only if the
node application is running with root privileges.
[Please read the class documentation for details](https://github.com/UAVCAN/libuavcan/blob/master/libuavcan_drivers/linux/include/uavcan_linux/clock.hpp).

## Installation

This part describes how to install libuavcan - including the Linux driver - on a Linux machine.

Runtime prerequisites:

* Linux kernel 2.6.33+ (recommended 3.9+)
* librt (available by default in all Linux distributions)

Build prerequisites:

* All runtime prerequisites
* Either [Python 2.7 or Python 3.x](https://www.python.org)
* C++11-capable compiler, e.g., GCC 4.8 or newer
* CMake 2.8+

Suggested packages (optional):

* can-utils

Build steps:

```sh
cd uavcan
mkdir build
cd build
cmake .. # Optionally, set the build type: -DCMAKE_BUILD_TYPE=Release (default is RelWithDebInfo)
make     # This  will invoke the DSDL compiler automatically
sudo make install
```

The following components will be installed on the system:

* The libuavcan static library and headers.
* The Linux driver for libuavcan (this is a header-only library, so only headers are installed).
* All standard DSDL type definitions (path: `<installation-prefix>/share/uavcan/dsdl/`).
* All generated headers for the standard DSDL types (installed into the same directory as the libuavcan headers).
* The [[pyuavcan]] package (will be installed for the default Python version).
* The libuavcan DSDL compiler - `libuavcan_dsdlc` (use `--help` to get usage info).
* Simple network monitoring tools - see below.

Installation prefix can be overridden via make variable `PREFIX`.
Default installation prefix depends on the Linux distribution;
for example, on Ubuntu the default prefix is `/usr/local/`.

## Usage

Documentation for each feature is provided in the Doxygen comments in the header files.

Linux applications that use libuavcan need to link the following libraries:

* libuavcan
* librt

### Code example

This section presents the code for a very simple node that gives a good idea on how to use libuavcan on Linux.
Also, take a look at the applications available at
[libuavcan_drivers/linux/apps](https://github.com/UAVCAN/libuavcan/tree/master/libuavcan_drivers/linux/apps).

#### CMake script

Place this into the file `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 2.8)
project(pan_galactic_gargle_blaster)
set(CMAKE_CXX_FLAGS "-std=c++11")
find_library(UAVCAN_LIB uavcan REQUIRED)
add_executable(pan_galactic_gargle_blaster node.cpp)
target_link_libraries(pan_galactic_gargle_blaster ${UAVCAN_LIB} rt)
```

#### C++ source file

Place this into the file `node.cpp`:

```c++
#include <iostream>
#include <vector>
#include <string>
#include <uavcan_linux/uavcan_linux.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/param/ExecuteOpcode.hpp>

static uavcan_linux::NodePtr initNode(const std::vector<std::string>& ifaces, uavcan::NodeID nid,
                                      const std::string& name)
{
    auto node = uavcan_linux::makeNode(ifaces);

    node->setNodeID(nid);
    node->setName(name.c_str());

    /*
     * Starting the node
     */
    if (0 != node->start())
    {
        throw std::runtime_error("Bad luck");
    }

    /*
     * Checking whether our node conflicts with other nodes. This may take a few seconds.
     */
    uavcan::NetworkCompatibilityCheckResult init_result;
    if (0 != node->checkNetworkCompatibility(init_result))
    {
        throw std::runtime_error("Bad luck");
    }
    if (!init_result.isOk())
    {
        throw std::runtime_error("Network conflict with node " + std::to_string(init_result.conflicting_node.get()));
    }

    return node;
}

static void runForever(const uavcan_linux::NodePtr& node)
{
    /*
     * Set the status to OK to inform other nodes that we're ready to work now
     */
    node->setStatusOk();

    /*
     * Subscribing to the logging message just for fun
     */
    auto log_sub = node->makeSubscriber<uavcan::protocol::debug::LogMessage>(
        [](const uavcan::ReceivedDataStructure<uavcan::protocol::debug::LogMessage>& msg)
        {
            std::cout << msg << std::endl;
        });

    /*
     * Key Value publisher
     */
    auto keyvalue_pub = node->makePublisher<uavcan::protocol::debug::KeyValue>();

    /*
     * Timer that uses the above publisher once a minute
     */
    auto timer = node->makeTimer(uavcan::MonotonicDuration::fromMSec(60000), [&](const uavcan::TimerEvent&)
        {
            uavcan::protocol::debug::KeyValue msg;
            msg.key = "some_message";
            uavcan::protocol::param::String str;
            str.value = "Nothing continues to happen";
            msg.value.value_string.push_back(str);
            (void)keyvalue_pub->broadcast(msg);
        });

    /*
     * A useless server that just prints the request and responds with a default-initialized response data structure
     */
    auto server = node->makeServiceServer<uavcan::protocol::param::ExecuteOpcode>(
        [](const uavcan::protocol::param::ExecuteOpcode::Request& req,
           uavcan::protocol::param::ExecuteOpcode::Response&)
        {
            std::cout << req << std::endl;
        });

    /*
     * Spinning forever
     */
    while (true)
    {
        const int res = node->spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            node->logError("spin", "Error %*", res);
        }
    }
}

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage:\n\t" << argv[0] << " <node-id> <can-iface-name-1> [can-iface-name-N...]" << std::endl;
        return 1;
    }
    const int self_node_id = std::stoi(argv[1]);
    std::vector<std::string> iface_names(argv + 2, argv + argc);
    auto node = initNode(iface_names, self_node_id, "org.uavcan.pan_galactic_gargle_blaster");
    std::cout << "Initialized" << std::endl;
    runForever(node);
    return 0;
}
```

#### Building and Running

In the same directory, execute this:

```sh
mkdir build
cd build
cmake ..
make
./pan_galactic_gargle_blaster 42 vcan0  # Args: <node-id>, <can-iface-name>
```

Hint: Refer to the [[Libuavcan_tutorials:_Node_initialization_and_startup#Adding_a_virtual_CAN_interface|libuavcan tutorials to learn how to add a virtual CAN bus interface for testing on Linux]].

### Note on SocketCAN

It is recommended to enable automatic bus-off recovery using the option `restart-ms`, as shown in an example below:

```sh
sudo ip link set can0 up type can bitrate 1000000 sample-point 0.875 restart-ms 100
```

This will ensure reliable operation of the dynamic node ID allocation procedure.

Learn more at [https://www.kernel.org/doc/Documentation/networking/can.txt https://www.kernel.org/doc/Documentation/networking/can.txt].

## Network monitoring tools

Libuavcan driver for Linux also includes a couple of simple CLI applications for network monitoring and control.

### `uavcan_monitor`

![](/Implementations/Libuavcan/figures/uavcan_monitor.png)

This application displays a list of online nodes, their health code, operational mode, and uptime in seconds.
The information is displayed as a continuously updating colorized ASCII table.

Note that this application does not require its own Node ID,
because it runs in passive mode (i.e., it can listen to the bus traffic, but can't transmit).

How to use:

    uavcan_monitor <can_iface_name> [can_iface_name ...]

Example for `can0`:

    uavcan_monitor can0

To exit the application, press **Ctrl**+**C**.

### `uavcan_dynamic_node_id_server`

![](/Implementations/Libuavcan/figures/uavcan_dynamic_node_id_server.png)

This application implements a dynamic node ID allocation server. Start without arguments to see usage info.

### `uavcan_nodetool`

This application allows to access basic functions of UAVCAN nodes, such as:

* Read or change remote node configuration parameters via standard services `uavcan.protocol.param.*`.
* Restart a remote node using `uavcan.protocol.RestartNode`.
* Request remote node info, such as:
  * `uavcan.protocol.GetNodeInfo`
  * `uavcan.protocol.GetTransportStats`
* Perform automated enumeration using `uavcan.protocol.EnumerationRequest`.
* Etc.

Usage info can be obtained directly from the application by typing "help" in its internal command line.

How to start:

    uavcan_nodetool <node_id> <can_iface_name> [can_iface_name ...]

Example for `can0` and Node ID 127:

    uavcan_nodetool 127 can0

To exit the application, press **Ctrl**+**C**.

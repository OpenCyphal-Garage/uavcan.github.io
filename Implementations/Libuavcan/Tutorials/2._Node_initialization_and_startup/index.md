---
---

# Node initialization and startup

In this tutorial we'll create a simple node that doesn't do anything useful yet.

## Abstract

In order to add UAVCAN functionality into an application, we'll have to create a node object of class `uavcan::Node`,
which is the cornerstone of the library's API.
The class requires access to the CAN driver and the system clock, which are platform-specific components,
therefore they are isolated from the library through the following interfaces:

* `uavcan::ICanDriver`
* `uavcan::ICanIface`
* `uavcan::ISystemClock`

Classes that implement above interfaces are provided by libuavcan platform drivers.
Applications that are using libuavcan can be easily ported between platforms by means of swapping the implementations
of these interfaces.

### Memory management

Libuavcan does not use heap.
Dynamic memory allocations are managed by constant-time determenistic fragmentation-free block memory allocator.
The size of the memory pool that is dedicated to block allocation should be defined compile-time through a template
argument of the node class `uavcan::Node`.

**TODO**

### Threading

**TODO**

## The code

Put the following code into `node.cpp`:

```c++
{% include_relative node.cpp %}
```

## Running on Linux

We need to implement the platform-specific functions declared in the beginning of the example code above,
write a build script, and then build the application.
For the sake of simplicity, it is assumed that there's a single CAN interface named `vcan0`.

Libuavcan must be installed on the system in order for the build to succeed.
Also take a look at the CLI tools that come with the libuavcan Linux platform driver -
they will be helpful when learning from the tutorials.

### Implementing the platform-specific functions

Put the following code into `platform_linux.cpp`:

```c++
{% include_relative platform_linux.cpp %}
```

### Writing the build script

We're going to use [CMake](http://cmake.org/), but other build systems will work just as well.
Put the following code into `CMakeLists.txt`:

```cmake
{% include_relative CMakeLists.txt %}
```

#### Building

The building is quite standard for CMake:

```sh
mkdir build
cd build
cmake ..
make -j4
```

When these steps are executed, the application can be launched.

### Adding a virtual CAN interface

It is convenient to test CAN applications against virtual CAN interfaces.
In order to add a virtual CAN interface, execute the following:

```sh
IFACE="vcan0"

modprobe can
modprobe can_raw
modprobe can_bcm
modprobe vcan

ip link add dev $IFACE type vcan
ip link set up $IFACE

ifconfig $IFACE up
```

#### Using `candump`

`candump` is a tool from the package `can-utils` that can be used to monitor CAN bus traffic.
If this package is not available for your Linux distribution, you can
[build it from sources](http://elinux.org/Can-utils).

```sh
candump -caeta vcan0
```

## Running on STM32

**TODO**

## Running on LPC11C24

**TODO**


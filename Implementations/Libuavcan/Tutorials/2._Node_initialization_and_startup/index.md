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

#### Dynamic memory

Dynamic memory allocations are managed by constant-time determenistic fragmentation-free block memory allocator.
The size of the memory pool that is dedicated to block allocation should be defined compile-time through a template
argument of the node class `uavcan::Node`.

The library uses dynamic memory for the following tasks:

* Allocation of temporary buffers for reception of multi-frame transfers;
* Keeping receiver states (refer to the transport layer specification for more info);
* Keeping transfer ID map (refer to the transport layer specification for more info);
* Prioritized TX queue;
* Some high-level logic.

Typically, the size of the memory pool will be between 4 KB (for a simple node) and 512 KB (for a very complex node).
The minimum safe size of the memory pool can be evaluated as a double of the sum of the following values:

* For every incoming data type, multiply its maximum serialized length to the number of nodes that may publish it and
add up the results.
* Sum the maximum serialized length of all outgoing data types and multiply to the number of CAN interfaces available
to the node plus one.

It is recommended to round the result up to 64-byte boundary.

##### Memory pool sizing example

Consider the following example:

Property                                            | Value
----------------------------------------------------|------------------------------------------------------------------
Number of CAN interfaces                            | 2
Number of publishers of the data type A             | 3
Maximum serialized length of the data type A        | 100 bytes
Number of publishers of the data type B             | 32
Maximum serialized length of the data type B        | 24 bytes
Maximum serialized length of the data type X        | 256 bytes
Maximum serialized length of the data type Z        | 10 bytes

A node that is interested in receiving data types A and B, and in publishing data types X and Y,
will require at least the following amount of memory for the memory pool:

    2 * ((3 * 100 + 32 * 24) + (2 + 1) * (256 + 10)) = 3732 bytes
    Round up to 64: 3776 bytes.

##### Memory gauge

The library allows to query current memory use as well as peak (worst case) memory use with the following methods of
the class `PoolAllocator<>`:

* `uint16_t getNumUsedBlocks() const` - returns the number of blocks that are currently in use.
* `uint16_t getNumFreeBlocks() const` - returns the number of blocks that are currently free.
* `uint16_t getPeakNumUsedBlocks() const` - returns the peak number of blocks allocated, i.e. worst case pool usage.

The methods above can be accessed as follows:

```c++
std::cout << node.getAllocator().getNumUsedBlocks()     << std::endl;
std::cout << node.getAllocator().getNumFreeBlocks()     << std::endl;
std::cout << node.getAllocator().getPeakNumUsedBlocks() << std::endl;
```

The example above assumes that the node object is named `node`.

#### Stack allocation

Deserialized message and service objects are allocated on the stack.
Therefore, the size of the stack available to the thread must account for the largest used message or service object.
This also implies that any reference to a message or service object passed to the application via a callback will be
invalidated once the callback returns control back to the library.

Typically, minimum safe size of the stack would be about 1.5 KB.

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


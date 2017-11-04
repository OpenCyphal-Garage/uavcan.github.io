---
weight: 0
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
If the template argument is not provided, the node will expect the application to provide a reference to a custom
allocator to the constructor (custom allocators are explained below).

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

```cpp
std::cout << node.getAllocator().getNumUsedBlocks()     << std::endl;
std::cout << node.getAllocator().getNumFreeBlocks()     << std::endl;
std::cout << node.getAllocator().getPeakNumUsedBlocks() << std::endl;
```

The example above assumes that the node object is named `node`.

##### Custom allocators

The node classes can accept a custom allocator provided by the application instead of using a default one.
In order to use a custom allocator, omit the template argument to the node class, and provide a reference to
`uavcan::IPoolAllocator` to the node class constructor.

The library implements an alternative allocator that uses heap memory instead of a statically allocated pool.
It can be found under `uavcan/helpers/heap_based_pool_allocator.hpp`.
Please read the class documentation for more info.

The tutorial dedicated to multithreading also covers the topic of custom allocators.

#### Stack allocation

Deserialized message and service objects are allocated on the stack.
Therefore, the size of the stack available to the thread must account for the largest used message or service object.
This also implies that any reference to a message or service object passed to the application via a callback will be
invalidated once the callback returns control back to the library.

Typically, minimum safe size of the stack would be about 1.5 KB.

### Threading

From the standpoint of threading, the following configurations are available:

* Single-threaded - the library runs entirely in a single thread.
* Multi-threaded - the library runs in two threads:
  * Primary thread, which is suitable for hard real time, low-latency communications;
  * Secondary threads, which are suitable for blocking, I/O-intensive, or CPU-intensive tasks, but not for
real-time tasks.

#### Single-threaded configuration

In a single-threaded configuration, the library's thread should either always block inside `Node<>::spin()`, or
invoke `Node<>::spin()` or `Node<>::spinOnce()` periodically.

##### Blocking inside `spin()`

This is the most typical use-case:

```cpp
for (;;)
{
    const int error = node.spin(uavcan::MonotonicDuration::fromMSec(100));
    if (error < 0)
    {
        std::cerr << "Transient error: " << error << std::endl; // Handle the transient error...
    }
}
```

Some background processing can be performed between the calls to `spin()`:

```cpp
for (;;)
{
    const int error = node.spin(uavcan::MonotonicDuration::fromMSec(100));
    if (error < 0)
    {
        std::cerr << "Transient error: " << error << std::endl; // Handle the transient error...
    }

    performBackgroundProcessing();  // Will be invoked between spin() calls, at 10 Hz in this example
}
```

In the latter example, the function `performBackgroundProcessing()` will be invoked at 10 Hz between calls to
`Node<>::spin()`.
The same effect can be achieved with libuavcan's timer objects, which will be covered in the following tutorials.

##### Blocking outside of `spin()`

This is a more involved use case.
This approach can be useful if the application needs to poll various I/O operations in the same thread
with libuavcan.

In this case, the application will have to obtain the *file descriptor* from the UAVCAN's CAN driver object,
and add it to the set of file descriptors it is polling on.
Whenever the CAN driver's file descriptor reports an event, the application will call either
`Node<>::spin()` or `Node<>::spinOnce()`.
Also, the spin method must be invoked periodically; the recommended maximum period is 10 milliseconds.

The difference between `Node<>::spin()` and `Node<>::spinOnce()` is as follows:

* `spin()` - blocks until the timeout has expired, then returns, even if some of the CAN frames or timer events
are still pending processing.
* `spinOnce()` - instead of blocking,
it returns immediately once all available CAN frames and timer events are processed.

#### Multi-threaded configuration

This configuration is covered later in a dedicated tutorial.
In short, it splits the node into multiple node objects,
where each node object is being maintained in a separate thread.
The same concepts concerning the spin methods apply as for the single-threaded configuration (see above).

Typically, the separated node objects communicate with each other by means of a *virtual CAN driver interface*.

## The code

Put the following code into `node.cpp`:

```cpp
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

```cpp
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

### Using virtual CAN interface

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

Libuavcan also includes utility `uavcan_add_vcan` that automates the procedure.
For example:

```bash
uavcan_add_vcan vcan0
```

Execute `uavcan_add_vcan --help` to get usage info.

### Using SLCAN (tunneling CAN over serial port)

Libuavcan includes an utility named `uavcan_add_slcan` that allows to easily configure an SLCAN interface.
For example:

```bash
uavcan_add_slcan /dev/ttyACM0
```

Execute `uavcan_add_slcan --help` to get full usage info.

Note that SLCAN support requires that the package `can-utils` is installed on the system.
On a Debian/Ubuntu-based system the package can be installed from APT: `apt-get install can-utils`.
If this package is not available for your Linux distribution, you can
[build it from sources](http://elinux.org/Can-utils).

In certain cases SLCAN adapters may be losing TX frames due to insufficient capacity of the interface's TX queue.
This can be easily solved by means of providing larger TX queue for the interface:

```sh
ifconfig slcan0 txqueuelen 100     # Where 'slcan0' is the interface and '100' is queue depth
```

### Using `candump`

`candump` is a tool from the package `can-utils` that can be used to monitor CAN bus traffic.

```sh
candump -caeta vcan0
```

You can always combine it with `grep` and other standard tools for advanced data processing, for example:

```sh
# Ignore status messages of node 121:
candump -caetz slcan0 | grep -v '\S\S015579'
# Print only messages from node 10 except its status messages:
candump -caetz slcan0 | grep '\S\S\S\S\S\S0A' | grep -v '\S\S01550A'
# Dump exchange to a file for later analysis:
candump -caetz slcan0 > can_`date -Iseconds`.dump
```

Learn how to grep from the
[grep manual](https://www.gnu.org/software/findutils/manual/html_node/find_html/grep-regular-expression-syntax.html).

## Running on STM32

The platform-specific functions can be implemented as follows:

```cpp
{% include_relative platform_stm32.cpp %}
```

For the rest, please refer to the STM32 test application provided in the repository.

## Running on LPC11C24

The platform-specific functions can be implemented as follows:

```cpp
{% include_relative platform_lpc11c24.cpp %}
```

For the rest, please refer to the LPC11C24 test application provided in the repository.


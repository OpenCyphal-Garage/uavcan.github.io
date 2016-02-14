---
weight: 0
---

# Time synchronization

This advanced-level tutorial shows how to implement network-wide time synchronization with libuavcan.

Two applications are implemented:

* Time synchronization slave - a very simple application that synchronizes the local clock with the network time.
* Time synchronization master - a full-featured master that can work with other redundant masters in the same network.
Note that in real applications the master's logic can be more complex, for instance,
if the local time source is not always available (e.g. like in GNSS receivers).

The reader is highly encouraged to build these example applications and experiment a little:

1. Start a master and some slaves and see how the slaves synchronize with the master.
2. Start more than one master (redundant masters) and see how they interact with each other and how the nodes
converge to the same master.

Please note that UAVCAN does not explicitly define algorithms used for clock frequency and phase adjustments,
which are implementation defined.
For instance, the Linux platform driver relies on the kernel function `adjtime()` to perform clock adjustments;
other platform drivers may implement a simple PLL.

This tutorial assumes that you have completed the previous tutorials and have a solid understanding
of the time synchronization algorithm defined in the relevant part of the UAVCAN specification.

## Slave

This application implements a time synchronization slave.

```cpp
{% include_relative slave.cpp %}
```

## Master

This application implements a time synchronization master.

```cpp
{% include_relative master.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

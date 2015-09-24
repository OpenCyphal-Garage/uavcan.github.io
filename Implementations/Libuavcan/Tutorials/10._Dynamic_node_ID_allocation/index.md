---
---

# Dynamic node ID allocation

This advanced-level tutorial demonstrates how to implement dynamic node ID allocation using libuavcan.
The reader must be familiar with the corresponding section of the specification, since the content of this
chapter heavily relies on principles and concepts introduced there.

In this tutorial, three applications will be implemented:

* **Allocatee** - a generic application that requests a dynamic node ID.
It does not implement any specific application-level logic.
* **Centralized allocator** - one of two possible types of allocators.
* **Distributed allocator** - the other type of allocators, that can operate in a highly reliable redundant cluster.
Note that the Linux platform driver contains an implementation of a distributed dynamic node ID allocator -
learn more on the chapter dedicated to the Linux platform driver.

## Allocatee

Use this example as a guideline when implementing dynamic node ID allocation feature in your application.

```c++
{% include_relative allocatee.cpp %}
```

Possible output:

```
No preference for a node ID value.
To assign a preferred node ID, pass it as a command line argument:
        ./allocatee <preferred-node-id>
major: 0
minor: 0
unique_id: [68, 192, 139, 99, 94, 5, 244, 188, 76, 138, 148, 82, 94, 146, 130, 178]
certificate_of_authenticity: ""
Allocation is in progress...................................................
Dynamic node ID 125 has been allocated by the allocator with node ID 1
```

## Centralized allocator

```c++
{% include_relative centralized_allocator.cpp %}
```

## Distributed allocator

```c++
{% include_relative distributed_allocator.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

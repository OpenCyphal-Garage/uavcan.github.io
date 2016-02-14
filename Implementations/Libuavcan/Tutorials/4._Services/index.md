---
weight: 0
---

# Services

This tutorial presents two applications:

* Server - provides a service of type `uavcan.protocol.file.BeginFirmwareUpdate`.
* Client - calls the same service type on a specified node.
Server's node ID is provided to the application as a command-line argument.

## Server

```cpp
{% include_relative server.cpp %}
```

## Client

```cpp
{% include_relative client.cpp %}
```

### Compatibility with older C++ standards

The following code demonstrates how to work-around the lack of important features in older C++ standards (prior C++11).

```cpp
{% include_relative client_cpp03.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

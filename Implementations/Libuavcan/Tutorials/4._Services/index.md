---
---

# Services

This tutorial presents two applications:

* Server - provides a service of type `uavcan.protocol.file.BeginFirmwareUpdate`.
* Client - calls the same service type on a specified node.
Server's node ID is provided to the application as a command-line argument.

## Server

```c++
{% include_relative server.cpp %}
```

## Client

```c++
{% include_relative client.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

---
weight: 0
---

# Publishers and subscribers

This tutorial presents two applications:

* Publisher, which broadcasts messages of type `uavcan.protocol.debug.KeyValue` once a second with random values.
* Subscriber, which subscribes to messages of types `uavcan.protocol.debug.KeyValue` and
`uavcan.protocol.debug.LogMessage`, and prints them to stdout in the [YAML](https://en.wikipedia.org/wiki/YAML) format.

Note that the presented publisher will not publish messages of type `uavcan.protocol.debug.LogMessage`.
The reader is advised to extend it with this functionality on their own.

## Publisher

```cpp
{% include_relative publisher.cpp %}
```

## Subscriber

```cpp
{% include_relative subscriber.cpp %}
```

### Compatibility with older C++ standards

The following code demonstrates how to work-around the lack of important features in older C++ standards (prior C++11).

```cpp
{% include_relative subscriber_cpp03.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

This build script assumes that the platform-specific functions are defined in
`../2._Node_initialization_and_startup/platform_linux.cpp`.

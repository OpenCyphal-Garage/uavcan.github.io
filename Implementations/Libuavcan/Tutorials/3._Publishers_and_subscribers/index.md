---
---

# Publishers and subscribers

The application presented in this tutorial performs the following:

* Subscribes to `uavcan.protocol.debug.LogMessage` and, for each received message, prints to stdout the following data:
  * The message itself formatted as YAML.
  * Transport layer data, such as Sender Node ID, timestamps, and active interface index.
These are formatted as a YAML comment.
* Subscribes to `uavcan.protocol.debug.KeyValue` and, for each received message,
prints the message data to stdout formatted as YAML (no transport layer data).
* Publishes a random float value via `uavcan.protocol.debug.KeyValue` once a second.

This tutorial assumes that you have completed the previous tutorials.

## The code

```c++
{% include_relative node.cpp %}
```

## Running on Linux

Build the application using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

This script assumes that the platform-specific functions are defined in
`../2._Node_initialization_and_startup/platform_linux.cpp`.

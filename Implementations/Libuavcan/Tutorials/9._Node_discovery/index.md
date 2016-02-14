---
weight: 0
---

# Node discovery

This tutorial demonstrates how to discover other nodes in the network and retrieve information about each node.
Two applications are implemented in this tutorial:

* Passive network monitor - a small application that simply listens to messages of type `uavcan.protocol.NodeStatus`,
which allows it to obtain the following minimal information about each node:
  * Node ID
  * Operating mode (Initialization, Operational, Maintenance, etc.)
  * Health code (OK, Warning, Error, Critical)
  * Uptime
* Active network monitor - an application that extends the passive monitor so that it actively requests
`uavcan.protocol.GetNodeInfo` whenever a node appears in the network or restarts.
This allows the monitor to obtain the following information about each node:
  * All information from the passive monitor (see above)
  * Node name
  * Software version information
  * Hardware version information, including the globally unique ID and the certificate of authenticity

Refer to the applications provided with the Linux platform drivers to see some
real-world examples of network monitoring.

## Passive monitor

```cpp
{% include_relative passive.cpp %}
```

## Active monitor

```cpp
{% include_relative active.cpp %}
```

The output may look like this:

{% include lightbox.html url="/Implementations/Libuavcan/Tutorials/9._Node_discovery/output.png" title="Sample output" %}

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

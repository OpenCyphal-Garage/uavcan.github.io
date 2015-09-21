---
---

# Remote node reconfiguration

This advanced-level tutorial shows how to do the following:

* Add support for standard configuration services to a node.
* Get, set, save, or erase configuration parameters on a remote node via UAVCAN.

These are the standard configuration services defined by the UAVCAN specification:

* `uavcan.protocol.param.GetSet`
* `uavcan.protocol.param.ExecuteOpcode`

Please read the specification for more info.

Two applications are implemented in this tutorial:

* Configurator - the node that can alter configuration parameters of a remote node via UAVCAN.
* Remote node - the node that supports remote reconfiguration.

## Configurator

Once started, this node performs the following:

* Reads all configuration parameters from a remote node (the user must enter the remote Node ID).
* Sets all parameters to their maximum values.
* Resets all parameters back to their default values.
* Restarts the remote node.
* Exits.

```c++
{% include_relative configurator.cpp %}
```

## Remote node

This node doesn't do anything on its own; it merely provides the standard configuration services.

```c++
{% include_relative remote_node.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

---
---

# Simple sensor node

This example is not based on any existing UAVCAN implementation -
it is completely standalone featuring zero third-party dependencies.

## Source code

```c
{% include_relative simple_sensor_node.c %}
```

## Testing against libuavcan

The following code can be used to test the node against the reference implementation of the UAVCAN stack.

```cpp
{% include_relative libuavcan_airspeed_subscriber.cpp %}
```

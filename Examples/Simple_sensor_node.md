---
---

# Simple sensor node

## Source code

```c
{% include_relative simple_sensor_node.c %}
```

## Testing against libuavcan

The following code can be used to test the node against the reference implementation of the UAVCAN stack.

```c++
{% include_relative libuavcan_airspeed_subscriber.cpp %}
```

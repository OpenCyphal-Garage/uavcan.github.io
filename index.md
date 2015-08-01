---
layout: home
---

# UAVCAN

Lightweight protocol designed for reliable communication in aerospace and robotic applications via [CAN bus](https://en.wikipedia.org/wiki/CAN_bus).

## Key features

- Very low data overhead - suitable for high frequency distributed control loops.
- Democratic network - no single point of failure.
- Supports network-wide time synchronization with microsecond precision.
- Allows to efficiently exchange long datagrams.
- Publish/subscribe or request/response exchange semantics.
- Doubly-redundant or triply-redundant CAN bus.
- Lightweight, easy to implement protocol.
- Can be used in deeply embedded systems (the reference implementation runs on a 32K ROM, 8K RAM MCU).
- The specification and the reference implementation are open and free to use ([MIT License](http://opensource.org/licenses/MIT)).

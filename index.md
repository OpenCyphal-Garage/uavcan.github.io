---
layout: home
---

**UAVCAN is an open lightweight protocol designed for reliable communication in aerospace and robotic
applications over robust vehicular networks such as CAN bus.**

- Democratic network -- no bus master, no single point of failure.
- Publish/subscribe and request/response (RPC) exchange semantics.
- Efficient exchange of large data structures with automatic decomposition and reassembly.
- Lightweight, deterministic, easy to implement, and easy to validate.
- Suitable for deeply embedded, resource constrained, hard real-time systems.
- Supports dual and triply modular redundant transports.
- Supports high-precision network-wide time synchronization.
- High-quality open source reference implementations are freely available under the MIT license.

This website is dedicated to the **experimental version** of the protocol, otherwise known as **UAVCAN v0**.
There is work underway to release the first stable long-term protocol specification and a set of
robust reference implementations by **2019 Q2**, at which point UAVCAN v0 will reach the end of its life cycle.
The new stable version that will replace it is known as **UAVCAN v1.0**.

**All new designs should avoid reliance on v0 and use UAVCAN v1.0 instead.**
Existing deployments will benefit from our commitment to provide long-term support and maintenance of
UAVCAN v0-based systems even after its EOL.

To learn more about the upcoming stable release UAVCAN v1.0,
please visit the **new website at [new.uavcan.org](https://new.uavcan.org/)**.

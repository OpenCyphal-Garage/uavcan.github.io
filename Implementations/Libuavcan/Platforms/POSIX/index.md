---
---

# POSIX helpers

This is not a complete driver,
but rather a set of C++ classes that implement certain libuavcan interfaces in a cross-platform way,
so that they can be used on any POSIX-compliant operating system,
e.g. Linux or NuttX.
The libuavcan tutorials cover how to use these classes among other things.

The driver is implemented as a header-only C++ library.
The sources can be found in
[`libuavcan_drivers/posix/`](https://github.com/UAVCAN/libuavcan/tree/master/libuavcan_drivers/posix).

## Features

At least the following classes are implemented in the POSIX driver:

* Firmware version checker - implements a simple firmware version checking strategy for firmware update servers.
* File server backend - implements file system I/O for UAVCAN file management services.
* File system backends for dynamic node ID allocation servers.

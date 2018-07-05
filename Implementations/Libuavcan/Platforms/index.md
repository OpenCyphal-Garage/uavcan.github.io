---
weight: 20
---

# Libuavcan platforms

In the context of Libuavcan, a *platform* is a particular hardware platform or an operating system that can
execute libuavcan-based UAVCAN nodes.
For example, the STM32 microcontroller family and the GNU/Linux OS are some of the officially supported
libuavcan platforms.

A *platform driver* is a set of platform-specific C++ classes that implement the thin layer of
glue logic needed to make libuavcan function with the particular platform at hand.

## Adding support for a new platform

Implementing a platform driver for libuavcan is generally a quite straightforward task,
especially if the driver is targeted for a certain application and need not to be generic.

Essentially, a libuavcan driver is a set of C++ classes implementing the following C++ interfaces:

* `uavcan::ICanDriver`
* `uavcan::ICanIface`
* `uavcan::ISystemClock`

The interfaces listed above are defined in
[`libuavcan/include/uavcan/driver/`](https://github.com/UAVCAN/libuavcan/blob/master/libuavcan/include/uavcan/driver).

Note that the library core already includes a non-blocking prioritized TX queue, so normally a driver should not
implement a software TX queue itself.

If the driver does not need to support redundant CAN bus interfaces,
then the first two C++ interfaces can be implemented in the same C++ class.

Some features can be left unimplemented:

* **IO flags** - needed for dynamic node ID allocation and time synchronization master.
* **TX timestamping** - needed for clock synchronization master.
* **Hardware CAN filters configuration** -
not necessary if there is enough computational power to delegate filtering to the library software.

There are two extreme examples of the driver complexity level that can give a basic idea of the development effort required:

* LPC11C24 driver - only necessary functionality, no redundant interface support, no TX timestamping - 600 LoC.
* Linux driver - complete full-featured driver with hardware timestamping and redundant interfaces - 1200 LoC.

Contributions adding support for new platforms are always welcome.

## Officially supported platforms

Official platform drivers are located in dedicated repositories here:
[github.com/UAVCAN](https://github.com/UAVCAN).
Contributions adding support for new platforms are always welcome.

Please refer to the platform driver source repositories for relevant documentation:

* [STM32](https://github.com/UAVCAN/libuavcan_stm32)
* [GNU/Linux](https://github.com/UAVCAN/libuavcan_linux)
* [NXP LPC11C24](https://github.com/UAVCAN/libuavcan_lpc11c24)

## POSIX helpers

This is not a complete driver,
but rather a set of C++ classes that implement certain libuavcan interfaces in a cross-platform way,
so that they can be used on any POSIX-compliant operating system,
e.g. Linux or NuttX.
The libuavcan tutorials available in an adjacent section cover how to use these classes among other things.

The driver is implemented as a header-only C++ library.
The sources can be found in the main libuavcan source repository.

At least the following classes are implemented in the POSIX driver:

* Firmware version checker - implements a simple firmware version checking strategy for firmware update servers.
* File server backend - implements file system I/O for UAVCAN file management services.
* File system backends for dynamic node ID allocation servers.

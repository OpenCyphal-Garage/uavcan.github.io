---
---

# Setup

This tutorial covers the basic setup of Pyuavcan.

Pyuavcan is designed to be as platform-independent as possible.
It has been successfully tested under Linux, Windows, and OSX,
but its portability should not be restricted only to these platforms.

## Installation

It is recommended to use PIP to install Pyuavcan:

```bash
pip install uavcan
```

If your operating system has both Python 2.7 and Python 3,
you probably should use `pip3` instead of `pip` in the example above.
Sometimes the PIP executable is not available at all,
in which case you should resort to the below command:

```bash
python -m pip install uavcan
```

Pyuavcan requires no external dependencies, except for PySerial if SLCAN support is required (more on this below).

## CAN hardware backends

Supported hardware backends are documented in this section.

### SLCAN (aka LAWICEL)

This backend is available under all supported operating systems.
In order to use it, you should also install PySerial:

```bash
pip install pyserial
```

SLCAN is a simple quasi-standard protocol that allows to transfer CAN frames and
CAN adapter commands and status information via a serial port link
(most often it's USB CDC ACM virtual serial port).
Many different vendors manufacture SLCAN-compatible CAN adapters.
Pyuavcan has been successfully tested at least with the following models:

* [Zubax Babel](https://zubax.com/products/babel)
* Thiemar muCAN
* VSCOM USB-CAN

Since SLCAN works over a serial port link, USB SLCAN adapters will be detected on the host
<abbr title="Operating System">OS</abbr> as a virtual serial port device.
Depending on your OS, serial ports can be accessed as follows:

* On Linux, all devices are represented as files; serial ports can be referred to using the following files:
    * `/dev/serial/by-id/*`, where `*` is a string descriptor of the connected device, e.g.
`/dev/serial/by-id/usb-Zubax_Robotics_Zubax_Babel_380032000D5732413336392000000000-if00`;
this is the recommended way of accessing serial ports on Linux,
because each connected device has a persistent name with <abbr title="Unique ID">UID</abbr>
that is invariant to the order in which devices are connected.
    * `/dev/ttyACM*` or `/dev/ttyUSB*`, where `*` is a number.
Files in this directory are symlinked to from `/dev/serial/` described above.
* On OSX, look for the following files:
    * `/dev/tty.usbmodem*`, where `*` is a number.
* On Windows, connected serial devices will be detected as COM ports.

### SocketCAN

The SocketCAN backend is available only under Linux.

For SocketCAN adapter configuration, please refer to the documentation that
came with your adapter. Once you have configured the adapter, it will be
available via a specific network interface name, e.g. `can0`.

It is also possible to use Pyuavcan with virtual CAN interfaces.
A virtual SocketCAN interface can be brought up using the following commands as root:

```bash
modprobe can
modprobe can_raw
modprobe can_bcm
modprobe vcan
ip link add dev vcan0 type vcan
ip link set up vcan0
ifconfig vcan0 up
```

The following devices have been tested successfully with the SocketCAN backend:

* 8devices USB2CAN
* PEAK-System PCAN-USB
* SLCAN adapters via the SLCAN bridge



---
permalink: /GUI_Tool/User_guide/
---

# User guide

This article provides a brief overview of the main capabilities of the UAVCAN GUI Tool.

## Supported CAN adapters

UAVCAN GUI Tool is based on PyUAVCAN, which at the moment supports at least the following hardware backends.

### SLCAN (LAWICEL) compatible adapters

This backend is available on all systems.
All adapters that implement the SLCAN (aka LAWICEL) protocol are supported.
The following adapters have been tested so far and are guaranteed to work:

* [Zubax Babel](https://zubax.com/products/babel) (recommended)
* Thiemar muCAN
* VSCOM USB-CAN

Any other SLCAN adapter should also work with UAVCAN GUI Tool without issues.

### Linux SocketCAN

[SocketCAN](https://en.wikipedia.org/wiki/SocketCAN) backend is available only on Linux systems.
It has been tested with the following adapters:

* 8devices USB2CAN
* PEAK System PCAN-USB
* [Zubax Babel](https://zubax.com/products/babel) (via SLCAN bridge)
* Virtual CAN interface

Any other adapter supported by SocketCAN should also work with UAVCAN GUI Tool without issues.

## Using Vendor-Specific DSDL Definitions

The UAVCAN GUI Tool is based on PyUAVCAN,
which will automatically scan the directory `~/uavcan_vendor_specific_types` for vendor-specific data types,
where `~` stands for the home directory of the current user, e.g. `/home/joe` or `C:\Users\Joe`.
Consider the following directory layout:

```
~
└── uavcan_vendor_specific_types
    └── sirius_cybernetics_corporation
        ├── 100.Foo.uavcan
        ├── 42.Bar.uavcan
        └── Baz.uavcan
```

The above layout defines the following custom data types:

* `sirius_cybernetics_corporation.Foo` with the default data type ID 100.
* `sirius_cybernetics_corporation.Bar` with the default data type ID 42.
* `sirius_cybernetics_corporation.Baz` where the default data type ID is not set.

## Interactive Console

The application embeds an IPython console (running Python 3.4 or newer)
that provides the user with access to the application's own UAVCAN node.

As the application is built on [PyUAVCAN](/Implementations/Pyuavcan),
all features of the library are avaiable to the user via the interactive shell.

The application exposes a few convenience functions and objects, which are listed at the top of the console
when it is started.
In order to get more information about any function or object, execute `help(object_name)`.
Autocompletion can be invoked by pressing the **Tab** key.

### Programming tips

**Never invoke blocking functions, such as `time.sleep()`, from the console.**
Instead, apply the practices of asynchronous programming;
the functions `defer()`, `periodic()`, and `stop()` should be of great help here.
See examples below.

Wrong:

```python
i = 470
t = -1

while i > 20:
    i *= 0.99
    c = int(i) + (1000 if t < 0 else 0)
    t = -t
    print ('command', c)
    broadcast(uavcan.equipment.hardpoint.Command(hardpoint_id=1, command=c))
    time.sleep(0.3)     # THIS IS WRONG
```

Same logic as above, implemented correctly:

```python
i = 470
t = -1

def run_once():
    global i, t
    i *= 0.99
    c = int(i) + (1000 if t < 0 else 0)
    t = -t
    print('command', c)
    broadcast(uavcan.equipment.hardpoint.Command(hardpoint_id=1, command=c))
    if i > 20:
        node.defer(0.3, run_once)       # Note: not blocking

node.defer(0, run_once)
```

Another example of a periodic process:

```python
import math, time

def publish_throttle_setpoint():
    sp_sin = int(512 * (math.sin(time.time()) + 2))
    sp_cos = int(512 * (math.cos(time.time()) + 2))
    message = uavcan.equipment.esc.RawCommand(cmd=[sp_sin, sp_cos, sp_sin, sp_cos])
    broadcast(message)

periodic(0.05, publish_throttle_setpoint)
```

All periodic processes, along with all subscriptions, can be stopped by executing `stop()`.

## Bus Monitor, Plotter, etc

UAVCAN GUI Tool provides a bunch of other features,
which are considered self-documenting and therefore not described here.
If you find that not true, please [file a ticket](https://github.com/UAVCAN/gui_tool)
or [contribute to this documentation](https://github.com/UAVCAN/uavcan.github.io).


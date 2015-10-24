---
---

# Basic setup

## SLCAN adapters

If your CAN adapter supports SLCAN, the device will typically be available at
one of the following paths:

* `/dev/tty.usbmodem<x>`
* `/dev/ttyACM<x>`
* `/dev/ttyUSB<x>`

The value of `<x>` will depend on your operating system and installed
hardware; if uncertain, try un-plugging the adapter, listing the `/dev`
directory, plugging it back in, and checking `/dev` for extra devices.

Install PySerial before continuing:

```sh
pip install pyserial
```

## SocketCAN adapters

For SocketCAN adapter configuration, please refer to the documentation that
came with your adapter. Once you have configured the adapter, it will be
available via a specific network interface name, e.g. `can0`.

## Running pyuavcan

Install pyuavcan, and open the Python interpreter:

```sh
pip install uavcan
python
```

Import the `uavcan` module, and load the built-in DSDL definitions:

```python
import uavcan
uavcan.load_dsdl()
```

Configure Python's built-in `logging` module to show messages in the
interactive shell:

```python
import logging
logging.root.setLevel(logging.DEBUG)
```

Now, import `uavcan.node`, instantiate a new `Node` object, and start
listening to bus traffic:

```python
import uavcan.node
node = uavcan.node.Node([])
node.listen('/your/device/name')
```

For SLCAN devices, the value of `/your/device/name` will be the `/dev/ttyX`
path you determined earlier. For SocketCAN devices, the value will be the
name of your SocketCAN network interface.

Once you call `node.listen`, you'll see a log line for each UAVCAN transfer
sent and received on the bus. The `Node` object will send a
`uavcan.protocol.NodeStatus` message at least once per second, and any other
devices on the network will do the same.

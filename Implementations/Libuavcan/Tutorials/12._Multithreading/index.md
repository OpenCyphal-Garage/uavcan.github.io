---
---

# Multithreading

This advanced-level tutorial explains the concept of multithreaded node in the context of libuavcan,
and shows an example application that runs a multithreaded node.

The reader must be familiar with the specification, particularly with the specification of the transport layer.

## Motivation

An application that runs a UAVCAN node is often required to run hard real-time UAVCAN-related processes
side by side with less time-critical ones.
Making both co-exist in the same execution thread can be challenging because of radically different requirements
and constraints.
For example, part of the application that controls actuators over UAVCAN may require very low latency,
whereas a non-realtime component (e.g. a firmware update trigger) does not require real-time priority,
but in turn may have to perform long blocking I/O operations, such as file system access, etc.

A radical solution to this problem is to run the node in two (or more) execution threads or processes.
This enables the designer to put hard real-time tasks in a dedicated thread, while running all low-priority and/or
blocking processes in lower-priority threads.

## Architecture

{% include lightbox.html url="/Implementations/Libuavcan/Tutorials/12._Multithreading/multithreading.svg" title="Multithreading with Libuavcan" thumbnail=true %}

Libuavcan allows to add low-priority threads by means of adding *sub-nodes*,
decoupled from the *main node* via a *virtual CAN driver*.
In this tutorial we'll be reviewing a use case with just one sub-node,
but the approach can be scaled to more sub-nodes if necessary by means of daisy-chaining them together
with extra virtual drivers.

A virtual CAN driver is a class that implements `uavcan::ICanDriver` (see libuavcan porting guide for details).
An object of this class is fed to the sub-node in place of a real CAN driver.

### Transmission

The sub-node emits its outgoing (TX) CAN frames into the virtual driver,
where they get enqueued in a synchronized prioritized queue.
The main node periodically unloads the enqueued TX frames from the virtual driver into its own
prioritized TX queue, which then gets flushed into the CAN driver, thus completing the pipeline.

Injection of TX frames from sub-node to the main node's queue is done via `uavcan::INode::injectTxFrame(..)`.

### Reception

The main node simply duplicates all incoming (RX) CAN frames to the virtual CAN driver.
This is done via the interface `uavcan::IRxFrameListener`,
which is installed via the method `uavcan::INode::installRxFrameListener(uavcan::IRxFrameListener*)`.

## Multiprocessing

{% include lightbox.html url="/Implementations/Libuavcan/Tutorials/12._Multithreading/multiprocessing.svg" title="Multiprocessing with Libuavcan" thumbnail=true %}

A node may be implemented not only in multiple threads, but in multiple processes as well
(with isolated address spaces).

In this case, every sub-node will be accessing the CAN hardware as an independent node,
but it will be using the same node ID in all communications, therefore all sub-nodes and the main node
will appear on the bus as the same network participant.

We introduce a new term here - *compound node* - which refers to either a multithreaded or a multiprocessed node.

## Limitations

A curious reader could have noticed that the above proposed approaches are in conflict
with requirements of the transport layer specification.
Indeed, there is one corner case that has to be kept in mind when working with compound nodes.

The transport layer specification introduces the concept of *transfer ID map*,
which is mandatory in order to assign proper transfer ID values to outgoing transfers.
The main node and its sub-nodes cannot use a shared transfer ID map, therefore
*multiple sub-nodes must not simultaneously publish transfers with identical descriptors*.

This point can be expressed in higher-level terms:

* Every sub-node of a compound node may receive any incoming transfers without limitations.
* Only one sub-node can implement a certain publisher or service server.

Note that the class `uavcan::SubNode` does not publish `uavcan.protocol.NodeStatus` and
does not provide service `uavcan.protocol.GetNodeInfo` and such (actually it doesn't implement any
publishers or servers or such at all), because this functionality is already provided by the main node class.

## Example

### Virtual CAN driver

```cpp
{% include_relative uavcan_virtual_driver.hpp %}
```

### Demo application

This demo also shows how to use a thread-safe heap-based shared block memory allocator.
This allocator enables lower memory footprint for compound nodes than the default one.

```cpp
{% include_relative node.cpp %}
```

Possible output:

```
uavcan_virtual_iface::Driver: Total memory blocks: 468, blocks per queue: 468
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   05 00 00 00 00 00 00 c5  '........' ts_m=120912.553326 ts_utc=1443574957.568273 iface=0
uavcan_virtual_iface::Iface: TX injection [iface_index=0]: <volat> 0x1e01ff81   c0  '.'
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   18 ef 05 00 00 00 00 80  '........' ts_m=120912.594184 ts_utc=1443574957.609083 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 00 00 00 00 00 00 20  '....... ' ts_m=120912.594188 ts_utc=1443574957.609096 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 00 00 00 00 00 00 00  '........' ts_m=120912.594189 ts_utc=1443574957.609102 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 00 00 00 00 00 00 20  '....... ' ts_m=120912.594189 ts_utc=1443574957.609110 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 00 00 00 00 00 00 00  '........' ts_m=120912.594190 ts_utc=1443574957.609116 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 00 00 00 00 00 00 20  '....... ' ts_m=120912.594191 ts_utc=1443574957.609121 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   00 6f 72 67 2e 75 61 00  '.org.ua.' ts_m=120912.594192 ts_utc=1443574957.609128 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   76 63 61 6e 2e 6c 69 20  'vcan.li ' ts_m=120912.594193 ts_utc=1443574957.609131 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   6e 75 78 5f 61 70 70 00  'nux_app.' ts_m=120912.594194 ts_utc=1443574957.609135 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   2e 6e 6f 64 65 74 6f 20  '.nodeto ' ts_m=120912.594195 ts_utc=1443574957.609139 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1e0101ff   6f 6c 40                 'ol@' ts_m=120912.594195 ts_utc=1443574957.609143 iface=0
Node info for 127:
status:
  uptime_sec: 5
  health: 0
  mode: 0
  sub_mode: 0
  vendor_specific_status_code: 0
software_version:
  major: 0
  minor: 0
  optional_field_flags: 0
  vcs_commit: 0
  image_crc: 0
hardware_version:
  major: 0
  minor: 0
  unique_id: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  certificate_of_authenticity: ""
name: "org.uavcan.linux_app.nodetool"
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   06 00 00 00 00 00 00 c6  '........' ts_m=120913.553339 ts_utc=1443574958.568299 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   07 00 00 00 00 00 00 c7  '........' ts_m=120914.553385 ts_utc=1443574959.568286 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   08 00 00 00 00 00 00 c8  '........' ts_m=120915.553300 ts_utc=1443574960.568262 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   09 00 00 00 00 00 00 c9  '........' ts_m=120916.553426 ts_utc=1443574961.568343 iface=0
uavcan_virtual_iface::Driver: RX [flags=0]: 0x1001557f   0a 00 00 00 00 00 00 ca  '........' ts_m=120917.553359 ts_utc=1443574962.568282 iface=0
Node 127 went offline
```

### Running on Linux

Build the application using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

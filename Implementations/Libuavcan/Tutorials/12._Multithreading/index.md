---
---

# Multithreading

Come back later.

## Virtual CAN driver

```c++
{% include_relative uavcan_virtual_iface.hpp %}
```

## Demo application

```c++
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

## Running on Linux

Build the application using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

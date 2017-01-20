---
---

# Firmware update

This advanced-level tutorial shows how to implement firmware update over UAVCAN using libuavcan.
The reader must be familiar with the corresponding section of the specification.
Two applications will be implemented:

* Updater - this application runs an active node monitor (this topic has been covered in one of the previous tutorials),
and when a remote node responds to `uavcan.protocol.GetNodeInfo` request, the application checks if it has a newer
firmware image than the node is currently running. If this is the case, the application sends a request of type
`uavcan.protocol.file.BeginFirmwareUpdate` to the node. This application also implements a file server, which is
another mandatory component of the firmware update process.
* Updatee - this application demonstrates how to handle requests of type `uavcan.protocol.file.BeginFirmwareUpdate`,
and how to download a file using the service `uavcan.protocol.file.Read`.

Launch instructions are provided after the source code below.

It is advised to refer to a real production-used cross-platform UAVCAN bootloader implemented by
[PX4 development team](http://px4.io).

## Updater

```cpp
{% include_relative updater.cpp %}
```

## Updatee

```cpp
{% include_relative updatee.cpp %}
```

## Running on Linux

Build the applications using the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```

If the applications are started as-is, nothing interesting would happen,
because updater needs firmware files in order to issue update requests;
otherwise it will be ignoring nodes with a comment like `No firmware files found for this node`.

In order to make something happen, create a file named `org.uavcan.tutorial.updatee-1.0-5.0.0.uavcan.bin`
(naming format is explained in the source code), fill it with some data and then start both nodes.

In this case, updater will request updatee to update its firmware, because the provided firmware file
is declared to have higher firmware version number than updatee reports.

While firmware is being loaded, updatee will set its operating mode to `SOFTWARE_UPDATE`,
as can be seen using UAVCAN monitor application for Linux
(refer to the Linux platform driver documentation to learn more about available applications):

{% include lightbox.html url="/Implementations/Libuavcan/Tutorials/11._Firmware_update/monitor_software_update.png" title="Sample" %}

Possible output of updater:

```
$ ./updater 1
Started successfully
Checking firmware version of node 2; node info:
status:
  uptime_sec: 0
  health: 0
  mode: 0
  sub_mode: 0
  vendor_specific_status_code: 0
software_version:
  major: 1
  minor: 0
  optional_field_flags: 0
  vcs_commit: 0
  image_crc: 0
hardware_version:
  major: 1
  minor: 0
  unique_id: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  certificate_of_authenticity: ""
name: "org.uavcan.tutorial.updatee"
Matching firmware files:
        org.uavcan.tutorial.updatee-1.0-5.0.0.uavcan.bin
status:
  uptime_sec: 0
  health: 0
  mode: 0
  sub_mode: 0
  vendor_specific_status_code: 0
software_version:
  major: 5
  minor: 0
  optional_field_flags: 1
  vcs_commit: 0
  image_crc: 0
hardware_version:
  major: 1
  minor: 0
  unique_id: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  certificate_of_authenticity: ""
name: "org.uavcan.tutorial.updatee"
Preferred firmware: org.uavcan.tutorial.updatee-1.0-5.0.0.uavcan.bin
Firmware file symlink: 93osmbx05mzk.bin
Node 2 has confirmed the update request; response was:
error: 0
optional_error_message: ""
```

Possible output of updatee:

```
$ ./updatee 2
Firmware update request:
# Received struct ts_m=9534.001067 ts_utc=1443463579.015996 snid=1
source_node_id: 1
image_file_remote_path:
  path: "93osmbx05mzk.bin"
Response:
error: 0
optional_error_message: ""
Firmware download succeeded [881 bytes]
00000000  54 68 65 20 74 72 61 67  65 64 79 20 6f 66 20 68   The tragedy of h
00000010  75 6d 61 6e 20 6c 69 66  65 2c 20 69 74 20 69 73   uman life, it is
00000020  20 6f 66 74 65 6e 20 74  68 6f 75 67 68 74 2c 20    often thought,
00000030  69 73 20 74 68 61 74 20  6f 75 72 20 6d 6f 72 74   is that our mort
00000040  61 6c 69 74 79 20 6d 65  61 6e 73 20 74 68 61 74   ality means that
00000050  20 64 65 61 74 68 20 69  73 20 74 68 65 20 6f 6e    death is the on
00000060  6c 79 20 74 68 69 6e 67  20 74 68 61 74 20 77 65   ly thing that we
00000070  20 6b 6e 6f 77 0a 66 6f  72 20 73 75 72 65 20 61    know.for sure a
00000080  77 61 69 74 73 20 75 73  2e 20 54 68 65 20 73 74   waits us. The st
00000090  6f 72 79 20 6f 66 20 56  69 74 61 6c 69 61 20 74   ory of Vitalia t
000000a0  75 72 6e 73 20 74 68 69  73 20 63 6f 6e 76 65 6e   urns this conven
000000b0  74 69 6f 6e 61 6c 20 77  69 73 64 6f 6d 20 6f 6e   tional wisdom on
000000c0  20 69 74 73 20 68 65 61  64 20 61 6e 64 20 73 75    its head and su
000000d0  67 67 65 73 74 73 20 74  68 61 74 20 69 6d 6d 6f   ggests that immo
000000e0  72 74 61 6c 69 74 79 0a  77 6f 75 6c 64 20 62 65   rtality.would be
000000f0  20 61 20 63 75 72 73 65  2e 20 57 65 20 6e 65 65    a curse. We nee
00000100  64 20 64 65 61 74 68 20  74 6f 20 67 69 76 65 20   d death to give
00000110  73 68 61 70 65 20 61 6e  64 20 6d 65 61 6e 69 6e   shape and meanin
00000120  67 20 74 6f 20 6c 69 66  65 2e 20 57 69 74 68 6f   g to life. Witho
00000130  75 74 20 69 74 2c 20 77  65 20 77 6f 75 6c 64 20   ut it, we would
00000140  66 69 6e 64 20 6c 69 66  65 20 70 6f 69 6e 74 6c   find life pointl
00000150  65 73 73 2e 20 4f 6e 20  74 68 69 73 0a 76 69 65   ess. On this.vie
00000160  77 2c 20 69 66 20 68 65  6c 6c 20 69 73 20 65 74   w, if hell is et
00000170  65 72 6e 61 6c 20 64 61  6d 6e 61 74 69 6f 6e 2c   ernal damnation,
00000180  20 74 68 65 20 65 74 65  72 6e 69 74 79 20 6f 66    the eternity of
00000190  20 6c 69 66 65 20 69 6e  20 48 61 64 65 73 20 77    life in Hades w
000001a0  6f 75 6c 64 20 62 65 20  65 6e 6f 75 67 68 20 74   ould be enough t
000001b0  6f 20 6d 61 6b 65 20 69  74 20 61 20 70 6c 61 63   o make it a plac
000001c0  65 20 6f 66 20 70 75 6e  69 73 68 6d 65 6e 74 2e   e of punishment.
000001d0  0a 0a 49 74 20 69 73 20  73 75 72 70 72 69 73 69   ..It is surprisi
000001e0  6e 67 20 68 6f 77 20 66  65 77 20 70 65 6f 70 6c   ng how few peopl
000001f0  65 20 77 68 6f 20 74 68  69 6e 6b 20 65 74 65 72   e who think eter
00000200  6e 61 6c 20 6c 69 66 65  20 77 6f 75 6c 64 20 62   nal life would b
00000210  65 20 64 65 73 69 72 61  62 6c 65 20 74 68 69 6e   e desirable thin
00000220  6b 20 68 61 72 64 20 61  62 6f 75 74 20 77 68 61   k hard about wha
00000230  74 20 69 74 20 77 6f 75  6c 64 20 65 6e 74 61 69   t it would entai
00000240  6c 2e 20 54 68 61 74 0a  69 73 20 75 6e 64 65 72   l. That.is under
00000250  73 74 61 6e 64 61 62 6c  65 2e 20 57 68 61 74 20   standable. What
00000260  77 65 20 70 72 69 6d 61  72 69 6c 79 20 77 61 6e   we primarily wan
00000270  74 20 69 73 20 73 69 6d  70 6c 79 20 6d 6f 72 65   t is simply more
00000280  20 6c 69 66 65 2e 20 54  68 65 20 65 78 61 63 74    life. The exact
00000290  20 64 75 72 61 74 69 6f  6e 20 6f 66 20 74 68 65    duration of the
000002a0  20 65 78 74 72 61 20 6c  65 61 73 65 20 69 73 20    extra lease is
000002b0  6e 6f 74 20 6f 75 72 20  70 72 69 6d 65 0a 63 6f   not our prime.co
000002c0  6e 63 65 72 6e 2e 20 49  74 20 64 6f 65 73 20 73   ncern. It does s
000002d0  65 65 6d 20 74 68 61 74  20 73 65 76 65 6e 74 79   eem that seventy
000002e0  20 79 65 61 72 73 2c 20  69 66 20 77 65 e2 80 99    years, if we...
000002f0  72 65 20 6c 75 63 6b 79  2c 20 69 73 6e e2 80 99   re lucky, isn...
00000300  74 20 6c 6f 6e 67 20 65  6e 6f 75 67 68 2e 20 54   t long enough. T
00000310  68 65 72 65 20 61 72 65  20 73 6f 20 6d 61 6e 79   here are so many
00000320  20 70 6c 61 63 65 73 20  74 6f 20 73 65 65 2c 20    places to see,
00000330  73 6f 20 6d 75 63 68 0a  74 6f 20 64 6f 20 61 6e   so much.to do an
00000340  64 20 65 78 70 65 72 69  65 6e 63 65 2e 20 49 66   d experience. If
00000350  20 6f 6e 6c 79 20 77 65  20 68 61 64 20 6d 6f 72    only we had mor
00000360  65 20 74 69 6d 65 20 74  6f 20 64 6f 20 69 74 21   e time to do it!
00000370  0a                                                 .
```

---
weight: 100
---

# Frequently asked questions

### Node is emitting incomplete transfers or losing frames

Possible causes are listed below.
Also
[refer to the first tutorial for more info on troubleshooting](/Implementations/Libuavcan/Tutorials/1._Library_build_configuration/#debugging-and-troubleshooting).

#### Memory pool is too small

This may prevent the node from allocating enough memory for the TX queue.
Try to increase the pool, and make sure that it is not getting exhausted.

#### Node is not spinning regularly

This may cause the frames scheduled for transmission to time out before even reaching the CAN driver.
Make sure that the spin method is being invoked regularly.

#### CAN frames lose arbitration

This could happen if the bus is too congested with higher-priority traffic.
Try to increase either or both:

- Transmission timeout.
- Transfer priority (if appropriate).

Both are explained in the relevant tutorials.

### Build is failing because of missing header files

This issue typically manifests itself with a message like that:

```
libuavcan/include/uavcan/time.hpp:12:32: fatal error: uavcan/Timestamp.hpp: No such file or directory
```

This could be caused by failing DSDL compilation - see below.

### DSDL compiler is failing

Make sure you have Python installed.

Inspect the build logs for error messages from the DSDL compiler.
They should be descriptive enough to help you understand the nature of the problem.

If you're facing the following error message:

```
  File "dsdl_compiler/libuavcan_dsdlc", line 59, in <module>
    from libuavcan_dsdl_compiler import run as dsdlc_run
  File "/home/jgoppert/git/libuavcan/libuavcan/dsdl_compiler/libuavcan_dsdl_compiler/__init__.py", line 17, in <module>
    from uavcan import dsdl
ImportError: No module named uavcan
```

Make sure that all git submodules are checked out and updated - use `git submodule update --init --recursive --force`.

### SocketCAN does not work reliably with an SLCAN adapter

Sometimes `slcand` may be losing outgoing frames because TX queue is not large enough.
TX queue of an interface can be resized as follows:

```bash
ifconfig slcan0 txqueuelen 1000
```

The example above makes TX queue of `slcan0` 1000 frames deep.

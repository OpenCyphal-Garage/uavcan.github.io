---
weight: 100
---

# Frequently asked questions

### Node is emitting incomplete transfers or missing frames

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

This could be caused by failing DSDL compilation.
Possible reasons for DSDL compilation failure are listed below.

#### Build system is unable to invoke the DSDL compiler

Make sure you have Python installed.

Make sure that all git submodules are checked out and updated - use `git submodule update --init --recursive --force`.

#### DSDL compiler is failing

Inspect the build logs for error messages from the DSDL compiler.
They should be descriptive enough to help you understand the nature of the problem.

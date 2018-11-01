---
weight: 0
---

# Libuavcan

{% include lightbox.html url="figures/libuavcan_data_flow.png" title="Libuavcan data flow" thumbnail=true %}

Libuavcan is a portable, cross-platform library written in C++ with minimal dependency on the C++ standard library.
It can be compiled by virtually any standard-compliant C++ compiler and can be used on virtually any architecture/OS.
Libuavcan is free to use for anyone under the terms of the MIT open source license.

Libuavcan is implemented in two components:

* Cross-platform **core**.
This is the largest component that implements the entire UAVCAN stack in a platform-independent way.
* Platform-specific **drivers**.
These are thin layers that adapt the core for a particular OS/architecture.
Drivers are separated from the core by a few C++ interface classes.
There are default drivers available for some popular platforms (e.g., Linux, STM32, LPC11C24).

The libuavcan core can be compiled in either C++03 or C++11 mode.
In the latter case, some extra features are enabled (e.g., callbacks will be implemented through `std::function`
instead of through raw function pointers).

In C++03 mode, the implementation has almost zero dependency on the C++ standard library,
which allows to use the library on platforms with very limited C++ support.
C++11 mode though requires many parts of the standard library (e.g., `<functional>`).
The library can detect the actual C++ standard in use at compile time,
but this can be overridden to force the older C++ standard if desired
(e.g., if the application is compiled in C++11 mode but the full standard library is not available).

The library supports a few compile-time configuration options via preprocessor symbols that allow to fine-tune
the implementation for a particular platform/application; in most cases though, it's not required because
there's either a safe default value or an auto-detect setting provided for each configuration option.

The implementation also features the following properties that render the library suitable
for deeply embedded and real-time systems:

* Zero use of the heap
(a custom built-in block memory allocator is implemented, which is deterministic and fragmentation free).
* No C++ exceptions are used by default, but the library can be configured via preprocessor symbols to throw
exceptions if a fatal error occurs (e.g., unexpected null pointer, boundary check failure, etc.).
* No run-time type identification (RTTI) is used.
* The source code is partially compliant with MISRA C++ 2008.
* The code is unit tested and validated using strong static analysis tools.

**The source code is available here: <https://github.com/UAVCAN/libuavcan>.**

## Build dependencies

The following software must be installed on the host computer in order to build the library:

* Either [Python 2.7 or Python 3.x](https://www.python.org) - needed for the DSDL compiler.
* A suitable C++ compiler (see below).
* Additional dependencies, if any, depending on the environment.
Please read the documentation related to your platform (refer to subsections) for more information.

### C++ compiler requirements

#### C++11

* IEEE754 floating point
* At least the following headers of the C++ standard library must be available:
  * `cassert`
  * `climits`
  * `cmath`
  * `cstddef`
  * `cstdio`
  * `cstdlib`
  * `cstring`
  * `cstdint`
  * `cfloat`
  * `cerrno`
  * `functional`

#### C++03

* IEEE754 floating point
* `long long` and `unsigned long long` integer types
* At least the following headers of the C++ standard library must be available:
  * `cassert`
  * `climits`
  * `cmath`
  * `cstddef`
  * `cstdio`
  * `cstdlib`
  * `cstring`
* The following headers of the C standard library must be available:
  * `float.h`
  * `stdint.h`
  * `stdio.h`

## DSDL compiler

Libuavcan has a DSDL compiler that converts DSDL definitions into libuavcan-compatible header files (`*.hpp`).
The compiler is titled `libuavcan_dsdlc`, and it is invoked as follows:

```sh
libuavcan_dsdlc source_dir
```

Where `source_dir` is the root namespace directory containing the data type definitions to be compiled,
e.g. `dsdl/uavcan`.
Please use `libuavcan_dsdlc --help` to obtain more detailed usage information.

You must invoke the DSDL compiler during your build process before compiling your application and/or the library.
For example, if you are using Make, a possible way to invoke the DSDL compiler is as follows:

```make
$(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR)))
UINCDIR += dsdlc_generated
```

For more information, refer to the examples specific to your target platform.

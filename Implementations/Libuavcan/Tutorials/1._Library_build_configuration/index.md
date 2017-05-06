---
weight: 0
---

# Library build configuration

Library build can be configured via C++ preprocessor definitions.
All configuration options are listed and documented in the header file
[`libuavcan/include/uavcan/build_config.hpp`](https://github.com/UAVCAN/libuavcan/blob/master/libuavcan/include/uavcan/build_config.hpp).

Note that all configuration options have either safe default values or autodetect settings;
so, in most cases, it's not necessary to change them.

Applications and the library should use exactly the same build configuration,
meaning that if the library is built separately from the application, e.g. as a static library,
the sets of preprocessor symbols used for the library configuration in both builds should match exactly.
Hence, if the library is built separately from the application,
it is highly recommended to use the default build configuration
(the defaults are smart enough to fit virtually any use case).
For instance, the static library build for Linux uses (and will always use) only the default build configuration.

## Library version number

The following definitions are guaranteed to be present in all future versions of the library:

* `UAVCAN_VERSION_MAJOR` - major library version number
* `UAVCAN_VERSION_MINOR` - minor library version number

These values can be used to work-around the API differences in different versions of the library.

## Build configuration options

Note that this chapter doesn't list some of the advanced options that are unlikely to be needed
in most use cases.
If you want to see the full list of configuration options, please refer to the header file `build_config.hpp`.

### `UAVCAN_CPP_VERSION`

This macro encodes whether the features that were introduced with newer versions of the C++ standard are supported.
It expands to the year number after which the standard was named:

* 2003 for C++03
* 2011 for C++11

Also, there are helper macros:

* `UAVCAN_CPP11` - expands to `2011`
* `UAVCAN_CPP03` - expands to `2003`

This option is automatically set according to the actual C++ standard used by the compiler,
which is detected via the standard symbol `__cplusplus`.
Sometimes, the user may want to force the library to use C++03 mode,
in which case this symbol can be overridden manually via compiler flags
(e.g. for GCC: `-DUAVCAN_CPP_VERSION=UAVCAN_CPP03`).

Learn more about the differences between standard versions on the [libuavcan page](/Implementations/Libuavcan).

### `UAVCAN_EXCEPTIONS`

This option defines how libuavcan handles fatal errors, e.g. range check error, unexpected null pointer, etc.

If this option is nonzero, libuavcan will throw a standard exception inherited from
`std::exception` every time it encounters a fatal error.
Possible exception types include `std::runtime_error` and `std::out_of_range`.
An exception description string (accessible via the method `what()`) indicates where the exception was thrown from.

If this option is zero, then in case of a fatal error libuavcan will perform the following steps in order:

1. Execute a zero assertion, so debug builds will abort here.
2. If possible, attempt to work around the error;
a possible case for such behavior is an out-of-bounds array access wherein the closest valid index is used
instead of the requested index.
3. If the previous step was not possible, the application will be terminated via `std::abort()`.

The default value will be 1 if the values of compiler-specific preprocessor symbols that indicate
whether exception handling is available are nonzero, e.g. `__EXCEPTIONS` or `_HAS_EXCEPTIONS`.
Otherwise, this option  defaults to 0.

### `UAVCAN_EXPORT`

This symbol is inserted before every symbol exported by the library. It is empty by default.

Possible use case, e.g. for GCC:

```cpp
#define UAVCAN_EXPORT    __attribute__((visibility ("default")))
```

### `UAVCAN_TINY`

This option is intended for very resource-constrained microcontrollers (<32 KB ROM or <8 KB RAM).
It removes some auxiliary library features in order to reduce memory footprint.

The default value is always zero (disabled).

Some of the important features affected in tiny mode:

* Transport layer self-diagnostics removed
* Logging removed
* The following services are not supported by default:
  * `uavcan.protocol.RestartNode`
  * `uavcan.protocol.GetDataTypeInfo`
  * `uavcan.protocol.GetTransportStats`

### `UAVCAN_TOSTRING`

If nonzero, this option adds a convenience string-conversion method, `std::string toString() const`,
to most of the library classes.

The default value is 0, unless any of the preprocessor symbols listed below is nonzero:

* `__linux__`
* `__linux`
* `__APPLE__`
* `_WIN64`
* `_WIN32`
* Possibly some else, refer to the implementation for the complete list

Note that this option is guaranteed to be disabled by default on deeply embedded systems.

### `UAVCAN_IMPLEMENT_PLACEMENT_NEW`

Set this option to nonzero if your C++ implementation does not implement the placement new operator.

The default value is zero.

### `UAVCAN_USE_EXTERNAL_SNPRINTF`

If this macro is nonzero, libuavcan will not define (but only declare) the function `uavcan::snprintf()`.
In this case the application will need to provide a custom implementation of this function,
otherwise it will fail to link.

If this macro is zero, `uavcan::snprintf()` will simply wrap `vsnprintf()` from the standard library.

This feature is useful for memory-constrained embedded systems,
where the standard `snprintf()` implementation is too large to fit in the memory.

The default value is zero.

### `UAVCAN_USE_EXTERNAL_FLOAT16_CONVERSION`

If this macro is nonzero, libuavcan will not define the functions that perform conversions between
the native `float` data type and the `float16` type used in DSDL,
allowing the application to link custom conversion routines, possibly taking advantage of
hardware support for floating point conversions (e.g. instructions `VCVTB` and `VCVTT` on ARM Cortex).
For more info refer to the file `libuavcan/src/marshal/uc_float_spec.cpp` or grep the sources.

If this macro is zero, the library will use generic conversion routines that are fully portable and hardware-agnostic.

The default value is zero.

### `UAVCAN_ASSERT(x)`

This function-like preprocessor macro is used to perform run-time checks in debug builds;
it works in the same way as the standard `assert()` macro.
It is mapped to the standard `assert()` macro by default.

If the symbol `UAVCAN_NO_ASSERTIONS` is set to a nonzero value,
this macro expands to an empty statement instead.
This allows to save sometimes up to 10 KB of ROM on some platforms and reduce CPU usage.

### `UAVCAN_MEM_POOL_BLOCK_SIZE`

Libuavcan uses a deterministic, fixed-size block allocator to store dynamic data.
This option allows to override the default block size to use a lower value,
which sometimes helps to reduce memory usage on platforms with a small pointer size (32 bits or less).

The memory pool block size must be a multiple of the biggest possible alignment on the target platform
(which is often the size of the largest primitive type, e.g., `long double`).

With some compilers (e.g., GCC), the library can adjust this value completely automatically using
the preprocessor symbol `__BIGGEST_ALIGNMENT__` so that the library user doesn't need to care.
In case automatic adjustment can't be performed, it defaults to a safe value, 64 bytes, which fits any platform.

If the block size is set too small for the current platform, the library will intentionally fail to compile.

### `UAVCAN_NO_GLOBAL_DATA_TYPE_REGISTRY`

This option will disable the global data type registry maintained by the library.
*Use of this feature is strongly discouraged*, unless you have a deep understanding of the library,
your application is extremely ROM and RAM limited, and you really know what you're doing.

If this option is enabled, the library will not be able to maintain the set of used DSDL data type
definitions automatically, so the developer will have to register every used data type manually
(this procedure is covered in one of the later tutorials).

Pros:

* Slightly lower ROM and RAM footprints of the library.
* The library will contain no singletons, and no static initialization will take place at start up.
* Inclusion of the data type definition headers will not affect the ROM footprint.

Cons:

* The developer will be responsible for registration of the DSDL data types needed by the application.

## Debugging and troubleshooting

The library facilitates debugging and troubleshooting through perfcounters and debug output.

### Perfcounters

#### `getFailureCount()`

Objects of the following types provide a method that allows the application to query the number of transport
layer failures detected by the protocol stack for the given service or message.

* `uavcan::Subscriber`
* `uavcan::ServiceServer`
* `uavcan::ServiceClient`

The signature of the method is `std::uint32_t getFailureCount() const`.

Detectable error types include:

* Transfer CRC failures
* Data type signature mismatch
* Data type layout mismatch
* Framing errors

#### `TransferPerfCounter`

Libuavcan maintains a set of performance counters that indicate the number of transfers processed,
as well as the number of common errors detected in the transport layer.
The performance counters can be accessed as follows:

```cpp
const uavcan::TransferPerfCounter& perf = node.getDispatcher().getTransferPerfCounter();

std::cout << perf.getErrorCount()      << std::endl;
std::cout << perf.getTxTransferCount() << std::endl;
std::cout << perf.getRxTransferCount() << std::endl;
```

The example above assumes that the node object is named `node`.

#### `CanIfacePerfCounters`

Libuavcan maintains the counters of CAN frames exchanged by the CAN controller and the number of
bus errors detected by the CAN hardware.

```cpp
const uavcan::CanIOManager& canio = node.getDispatcher().getCanIOManager();
for (std::uint8_t i = 0; i < canio.getNumIfaces(); i++)
{
    const uavcan::CanIfacePerfCounters can_perf = canio.getIfacePerfCounters(i);
    std::cout << can_perf.errors    << std::endl;
    std::cout << can_perf.frames_tx << std::endl;
    std::cout << can_perf.frames_rx << std::endl;
}
```

The example above assumes that the node object is named `node`.

### Memory pool usage statistics

Use the following methods to check whether the memory pool is getting exhausted:

```cpp
std::cout << node.getAllocator().getNumBlocks() << std::endl;           // Capacity
std::cout << node.getAllocator().getNumUsedBlocks() << std::endl;       // Currently used
std::cout << node.getAllocator().getPeakNumUsedBlocks() << std::endl;   // Peak usage
```

The example above assumes that the node object is named `node`.

### Debug output

The library will print extensive debug information into stdout via `std::printf()`
if the macro `UAVCAN_DEBUG` is assigned a non-zero value.
Note that this feature requires C++ I/O streams and `UAVCAN_TOSTRING`.


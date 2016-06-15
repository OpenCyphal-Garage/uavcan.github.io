---
---

# Libuavcan for STM32

This page describes the Libuavcan driver for the [STM32 MCU series](http://www.st.com/stm32) and shows how to use
libuavcan on this platform.

The libuavcan driver for STM32 is a C++ library with no third-party dependencies that provides bare-metal
drivers for the following:

* Built-in CAN interfaces (singly or doubly redundant, depending on the MCU model).
* One of the general-purpose timers (TIM2...TIM7); the driver also supports clock synchronization.

The following RTOS are supported:

* [ChibiOS](http://chibios.org/)
* [NuttX](http://nuttx.org/)
* [FreeRTOS](http://freertos.org/)
* Bare metal (no OS)

This driver should work with any STM32 series MCU. The following models were tested so far:

* STM32F103
* STM32F105
* STM32F107
* STM32F205
* STM32F427
* STM32F446

Virtually any standard-compliant C++ compiler for ARM Cortex-Mx can be used.
The library was mainly tested with GCC 4.7 and newer so far, but any other compiler should work too.

This driver can be used as a good starting point if a custom driver needs to be implemented for a
particular application.

## Building

No extra dependencies are required. The library need not be installed on the host system.

### Using Make

The general approach is as follows:

1. Include the libuavcan core makefile (`include.mk`) to obtain the following:
  1. List of C++ source files - make variable `$(LIBUAVCAN_SRC)`
  2. C++ include directory path - make variable `$(LIBUAVCAN_INC)`
  3. DSDL compiler executable path (which is a Python script) - make variable `$(LIBUAVCAN_DSDLC)`
2. Include the libuavcan STM32 driver makefile (`include.mk`) to obtain the following:
  1. List of C++ source files - make variable `$(LIBUAVCAN_STM32_SRC)`
  2. C++ include directory path - make variable `$(LIBUAVCAN_STM32_INC)`
3. Invoke the DSDL compiler using the variable `$(LIBUAVCAN_DSDLC)`.
4. Add the output directory of the DSDL compiler to the list of include directories.
5. Add the obtained list of C++ source files and include directories to the application sources or
build an independent static library.

The relevant part of the makefile implementing the steps above is provided below.
This example assumes that the UAVCAN repository is available in the same directory where the application's
makefile resides (e.g., as a git submodule).

```make
#
# Application
#

CPPSRC = src/main.cpp   # Application source files

UINCDIR = include       # Application include directories

UDEFS = -DNDEBUG        # Application preprocessor definitions

#
# UAVCAN library
#

UDEFS += -DUAVCAN_STM32_CHIBIOS=1      \ # Assuming ChibiOS for this example
         -DUAVCAN_STM32_TIMER_NUMBER=6 \ # Any suitable timer number
         -DUAVCAN_STM32_NUM_IFACES=2     # Number of CAN interfaces to use (1 - use only CAN1; 2 - both CAN1 and CAN2)

# Add libuavcan core sources and includes
include uavcan/libuavcan/include.mk
CPPSRC += $(LIBUAVCAN_SRC)
UINCDIR += $(LIBUAVCAN_INC)

# Add STM32 driver sources and includes
include uavcan/libuavcan_drivers/stm32/driver/include.mk
CPPSRC += $(LIBUAVCAN_STM32_SRC)
UINCDIR += $(LIBUAVCAN_STM32_INC)

# Invoke DSDL compiler and add its default output directory to the include search path
$(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR)))
UINCDIR += dsdlc_generated      # This is where the generated headers are stored by default

#
# ...makefile continues
#
```

### Using other build systems

One possible approach is the following:

* Manually add the libuavcan core include directory to the project includes.
The path is `uavcan/libuavcan/include`.
* Manually add the STM32 driver include directory to the project includes.
The path is `uavcan/libuavcan_drivers/stm32/driver/include`.
* Manually invoke the DSDL compiler and add the output directory to the project includes.
Alternatively, configure the build system to invoke the compiler automatically.
* Add all the C++ source files of libuavcan core and STM32 driver to the project.

## Build configuration

The driver is configurable via preprocessor definitions.
Some definitions must be provided in order for the build to succeed;
some others have default values that can be overridden only if needed.

The most important configuration options are documented in the table below.

Name                            | RTOS              | Description
--------------------------------|-------------------|--------------------------------------------------------------------------
`UAVCAN_STM32_CHIBIOS`          | ChibiOS           | Set to 1 if using ChibiOS.
`UAVCAN_STM32_NUTTX`            | NuttX             | Set to 1 if using NuttX.
`UAVCAN_STM32_FREERTOS`         | FreeRTOS          | Set to 1 if using FreeRTOS.
`UAVCAN_STM32_BAREMETAL`        | N/A               | Set to 1 if not using any OS.
`UAVCAN_STM32_IRQ_PRIORITY_MASK`| ChibiOS, FreeRTOS | This option defines IRQ priorities for the CAN controllers and the timer. Default priority level is one lower than the kernel priority level (i.e. max allowed level).
`UAVCAN_STM32_TIMER_NUMBER`     | Any               | Hardware timer number reserved for clock functions. If this value is not set, the timer driver will not be compiled; this allows the application to implement a custom driver if needed. Valid timer numbers are 2, 3, 4, 5, 6, and 7.
`UAVCAN_STM32_NUM_IFACES`       | Any               | Number of CAN interfaces to use. Valid values are 1 and 2.

## Examples

The following firmware projects can be used as a reference:

* [Zubax GNSS](https://github.com/Zubax/zubax_gnss)
* [PX4](https://github.com/PX4/Firmware)
* [PX4ESC](https://github.com/PX4/px4esc)
* Also see test applications available with the driver sources.

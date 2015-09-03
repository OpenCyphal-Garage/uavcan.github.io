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

* Include the libuavcan core makefile (`include.mk`) to obtain the following:
  * List of C++ source files - make variable `$(LIBUAVCAN_SRC)`
  * C++ include directory path - make variable `$(LIBUAVCAN_INC)`
  * DSDL compiler executable path (which is a Python script) - make variable `$(LIBUAVCAN_DSDLC)`
* Include the libuavcan STM32 driver makefile (`include.mk`) to obtain the following:
  * List of C++ source files - make variable `$(LIBUAVCAN_STM32_SRC)`
  * C++ include directory path - make variable `$(LIBUAVCAN_STM32_INC)`
* Invoke the DSDL compiler using the variable `$(LIBUAVCAN_DSDLC)`.
* Add the output directory of the DSDL compiler to the list of include directories.
* Add the obtained list of C++ source files and include directories to the application sources or
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

## Using in the application

This example demonstrates how to use libuavcan in an STM32 application.
It assumes that the application is running on a ChibiOS environment,
but it can be adapted to any other supported RTOS easily.

```c++
#include <unistd.h>
#include <ch.hpp>
#include <hal.h>
#include <uavcan_stm32/uavcan_stm32.hpp>

namespace app
{
namespace
{

constexpr unsigned CanBusBitrate = 1000000;

constexpr unsigned NodeID = 42;

constexpr unsigned NodeMemoryPoolSize = 4096 * (UAVCAN_STM32_NUM_IFACES + 1);

typedef uavcan::Node<NodeMemoryPoolSize> Node;

uavcan_stm32::CanInitHelper<> stm32_can_init_helper;

Node& getNode()
{
    // For GCC users: please use the flag -fno-threadsafe-statics in order to avoid code bloat here.
    // More info: http://stackoverflow.com/questions/22985570/g-using-singleton-in-an-embedded-application
    static Node node(stm32_can_init_helper.driver, uavcan_stm32::SystemClock::instance());
    return node;
}

int init()
{
    halInit();
    chibios_rt::System::init();
    sdStart(&STDOUT_SD, NULL);
    return stm32_can_init_helper.init(CanBusBitrate);
}

class : public chibios_rt::BaseStaticThread<3000>
{
public:
    msg_t main() override
    {
        Node& node = app::getNode();
        node.setNodeID(NodeID);
        node.setName("org.uavcan.kill_o_zap");
        // TODO: initialize the hardware and software version info

        while (true)
        {
            int res = node.start();
            if (res < 0)
            {
                printf("Node initialization failure: %i, will try agin soon\n", res);
            }
            else
            {
                break;
            }
            ::sleep(1);
        }

        node.setStatusOk();
        while (true)
        {
            const int spin_res = node.spin(uavcan::MonotonicDuration::getInfinite());
            if (spin_res < 0)
            {
                printf("Transient failure: %i\n", spin_res);
            }
        }
        return msg_t();
    }
} uavcan_node_thread;

}
}

int main()
{
    const int init_res = app::init();
    if (init_res != 0)
    {
        // Probably wrong configuration params, e.g. wrong CAN bitrate. Do something about it.
    }

    app::uavcan_node_thread.start(LOWPRIO);

    while (true)
    {
        ::sleep(1);
        // Doing something here
    }
}
```

## Build configuration

The driver is configurable via preprocessor definitions.
Some definitions must be provided in order for the build to succeed;
some others have default values that can be overridden only if needed.

The most important configuration options are documented in the table below.

Name                            | RTOS    | Description
--------------------------------|---------|--------------------------------------------------------------------------
`UAVCAN_STM32_CHIBIOS`          | ChibiOS | Set to 1 if using ChibiOS.
`UAVCAN_STM32_NUTTX`            | NuttX   | Set to 1 if using NuttX.
`UAVCAN_STM32_IRQ_PRIORITY_MASK`| ChibiOS | This option defines IRQ priorities for the CAN controllers and the timer. Default priority level is one lower than the kernel priority level (i.e. max allowed level).
`UAVCAN_STM32_TIMER_NUMBER`     | Any     | Hardware timer number reserved for clock functions. If this value is not set, the timer driver will not be compiled; this allows the application to implement a custom driver if needed. Valid timer numbers are 2, 3, 4, 5, 6, and 7.
`UAVCAN_STM32_NUM_IFACES`       | Any     | Number of CAN interfaces to use. Valid values are 1 and 2.

## Examples

The following firmware projects can be used as a reference:

* [Zubax GNSS](https://github.com/Zubax/zubax_gnss)
* [PX4](https://github.com/PX4/Firmware)
* [PX4ESC](https://github.com/pavel-kirienko/px4esc)
* Also see test applications available with the driver sources.

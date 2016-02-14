---
weight: 0
---

# Custom data types

This tutorial covers how to define and use vendor-specific data types (DSDL definitions).

The following topics are explained:

* How to define a vendor-specific data type.
* How to allow the end user to assign/override a data type ID for a certain type.
* How to invoke the DSDL compiler to generate C++ headers for custom data types.

Two applications are implemented in this tutorial:

* Server - the node that provides two vendor-specific services.
* Client - the node that calls the vendor-specific services provided by the server.

This tutorial requires the reader to be familiar with UAVCAN specification and
to have completed all the basic tutorials.

## Defining data types

### Abstract

Suppose we have a vendor named "Sirius Cybernetics Corporation" for whom
we need to define the following vendor-specific data types:

* `sirius_cybernetics_corporation.GetCurrentTime` - returns the current time on the server.
The default data type ID for this service is 242.
* `sirius_cybernetics_corporation.PerformLinearLeastSquaresFit` -
accepts a set of 2D coordinates and returns the coefficients for the best-fit linear function.
This service does not have a default data type ID.

Note that both data types are located in the namespace `sirius_cybernetics_corporation`,
which unambiguously indicates that these types are defined by Sirius Cybernetics Corporation and
allows to avoid name clashing if similarly named data types from different vendors are used in the same application.
Note that, according to the naming requirements, name of a DSDL namespace must start with an alphabetic character;
therefore, a company whose name starts with a digit will have to resort to a mangled name
(e.g., "42 Computing" &rarr; `fourtytwo_computing`, or `computing42`, etc.).

Even though this tutorial deals with service types,
the same concepts and procedures are also applicable to message types.
The reader is encouraged to extend this tutorial with at least one vendor-specific message data type.

### The procedure

As explained in the specification, a namespace for DSDL definitions is just a directory,
so we need to create a directory named `sirius_cybernetics_corporation`.
This directory can be placed anywhere; normally you'd probably want to put it in your project's root directory,
as we'll do in this tutorial.
You should not create it inside the libuavcan's DSDL directory (`libuavcan/dsdl/...`)
in order to not pollute the libuavcan's source tree with your custom data types.

In the newly created directory, place the following DSDL definition in a file named `242.GetCurrentTime.uavcan`:

```python
{% include_relative sirius_cybernetics_corporation/242.GetCurrentTime.uavcan %}
```

Note that the filename starts with the number 242 followed by a dot,
which tells the DSDL compiler that the default data type ID for this type is 242.
You can learn more about naming in the relevant part of the specification.

In the same directory, place the following DSDL definition in a file named `PerformLinearLeastSquaresFit.uavcan`:

```python
{% include_relative sirius_cybernetics_corporation/PerformLinearLeastSquaresFit.uavcan %}
```

Now, observe that the definition above refers to the data type `PointXY`.
Let's define it - place the following in a file named `PointXY.uavcan`:

```python
{% include_relative sirius_cybernetics_corporation/PointXY.uavcan %}
```

### Compiling

Normally, the compilation should be performed by the build system, which is explained later in this tutorial.
For the sake of a demonstration, let's compile the data types defined above by manually invoking the DSDL compiler.

If the host OS is Linux and libuavcan is installed, the following command can be used:

    $ libuavcan_dsdlc ./sirius_cybernetics_corporation -I/usr/local/share/uavcan/dsdl/uavcan

Alternatively, if the library is not installed, use relative paths
(the command invocation example below assumes that the libuavcan directory and
our vendor-specific namespace directory are both located in the project's root):

    $ libuavcan/libuavcan/dsdl_compiler/libuavcan_dsdlc ./sirius_cybernetics_corporation -Iuavcan/dsdl/uavcan/

Note that if the output directory is not specified explicitly via the command line option `--outdir` or `-O`,
the default will be used, which is `./dsdlc_generated`.
It is recommended to exclude the output directory from version control,
e.g. for git, add `dsdlc_generated` to the `.gitignore` config file.

## The code

### Server

```cpp
{% include_relative server.cpp %}
```

### Client

```cpp
{% include_relative client.cpp %}
```

## Building

This example shows how to build the above applications and compile the vendor-specific data types using CMake.
It's quite easy to adapt it to other platforms and build systems - use the
[existing projects](https://www.google.com/?q=site%3Agithub.com%20%2B%22uavcan%22%20%22Makefile%22) as a reference.

It is assumed that the libuavcan directory and our vendor-specific namespace directory are both located
in the project's root, although it is not necessary.

`CMakeLists.txt`:

```cmake
{% include_relative CMakeLists.txt %}
```

## Running

These instructions assume a Linux environment.
First, make sure that the system has at least one CAN interface.
You may want to refer to the first tutorials to learn how to add a virtual CAN interface on Linux.

Running the server with node ID 111:

    $ ./server 111

The server just sits there waiting for requests, not doing anything on its own.
Start another terminal and execute the client with node ID 112:

    $ ./client 112 111

The client should print something like this:

```
# Service call result [sirius_cybernetics_corporation.GetCurrentTime] OK server_node_id=111 tid=0
# Received struct ts_m=25935.913686 ts_utc=1442844327.400572 snid=111
time:
  usec: 1442844327400560
# Service call result [sirius_cybernetics_corporation.PerformLinearLeastSquaresFit] OK server_node_id=111 tid=0
# Received struct ts_m=25935.913891 ts_utc=1442844327.400725 snid=111
slope: 0.4
y_intercept: -4

```

Now, execute the client providing a non-existent server Node ID and see what happens:

```
$ ./client 112 1
# Service call result [sirius_cybernetics_corporation.GetCurrentTime] FAILURE server_node_id=1 tid=0
# (no data)
# Service call result [sirius_cybernetics_corporation.PerformLinearLeastSquaresFit] FAILURE server_node_id=1 tid=0
# (no data)
```

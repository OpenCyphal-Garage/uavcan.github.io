---
---

# Basic usage

This tutorial will walk you through the basic usage of Pyuavcan.
It is intended for execution in an interactive shell of Python 3.4 or newer.
While Pyuavcan supports Python 2.7, its use is strongly discouraged.

It is recommended to keep a running instance of the [UAVCAN GUI Tool](/GUI_Tool) on the same bus
while trying the examples below.
If you're working on Linux, consider using a virtual CAN interface for experimenting.

This guide is not an extensive list of all capabilities of the library.
For that, use the Python's help system (function `help()`), or read the code.

## Node initialization

Start a Python 3 shell and run this:

```python
import uavcan
node = uavcan.make_node('/dev/ttyACM0')
```

This will create a node connected to the SLCAN interface `/dev/ttyACM0`,
where all options are initialized to default values.
The library will select the correct backend automatically by looking at the name of the interface.
That is, if the interface is named like `COM3` or like above, the library will choose the SLCAN backend;
if the interface is named in the form of `can0`, the library will select SocketCAN.

A more useful node instantiation example is shown below:

```python
import uavcan

# Instantiating an instance of standard service type uavcan.protocol.GetNodeInfo
# Here we need the response data structure, which is why we use the factory Response()
node_info = uavcan.protocol.GetNodeInfo.Response()
node_info.name = 'org.uavcan.pyuavcan_demo'
node_info.software_version.major = 1
node_info.hardware_version.unique_id = b'12345' # Setting first 5 bytes; rest will be kept zero
# Fill other fields as necessary...

node = uavcan.make_node('vcan0',
                        node_id=123,          # Setting the node ID 123
                        node_info=node_info)  # Setting node info

# Alternatively, node ID can be set after initialization,
# but only if it hasn't been set before:
node.node_id = 123
```

While the node is running, it is possible to change its mode, health, and vendor specific status code
as follows:

```python
node.mode = uavcan.protocol.NodeStatus().MODE_OPERATIONAL
node.health = uavcan.protocol.NodeStatus().HEALTH_WARNING
node.vendor_specific_status_code = 12345
```

The library expects that it nearly always has a thread of execution blocked inside the
`spin()` method of the node class.
It is allowed to deviate from this requirement if the application does not have strict
real-time requirements by temporarily dispatching control to other tasks,
or by invoking the spin method periodically (at least 100 times per second) in a non-blocking way
with zero timeout.
Thus, the application should always keep one thread in the node's `spin()` function.

```python
while True:
    try:
        node.spin()             # Spin forever or until an exception is thrown
    except UAVCANException as ex:
        print('Node error:', ex)
```

Another popular use case:

```python
while True:
    try:
        node.spin(1)            # Spin for 1 second or until an exception is thrown
        perform_other_tasks()
        # time.sleep(0.1)       # Never do this!
    except UAVCANException as ex:
        print('Node error:', ex)
```

In general, if the application needs to perform other blocking tasks,
a separate thread should be spawned.
In the next section we'll learn how to run non-blocking parallel tasks from the node thread
by using the callback scheduler embedded in the node.

When you're finished with the node, don't forget to call `close()` on it; better yet, use contextlib:

```python
from contextlib import closing
with closing(uavcan.driver.make_node('/dev/ttyACM0', bitrate=1000000)) as node:
    # Do some work here...
# When control reaches the end of the with block, the node will be properly finalized
```

## Invoking callbacks from the node thread

{% include lightbox.html url="/Implementations/Pyuavcan/Tutorials/2._Basic_usage/gui_tool_node_info.png" title="Test node as seen from GUI Tool" thumbnail=true %}

In order to keep the node running in the background while having the shell available for
input, let's spawn a thread and dispatch it into the `spin()` method.
This approach works fine for experimenting, however,
it must never be used in production because the library is not thread safe.

```python
import threading
threading.Thread(target=node.spin, daemon=True).start()
```

It is recommended to launch the UAVCAN GUI tool now to make sure that the node can be seen online.

Now we'll see how to invoke callbacks once or periodically from the node thread:

```python
def periodic_callback_1hz():
    print('Periodic')

def deferred_call():
    print('Deferred call')

# Invoking a function every second.
periodic_handle = node.periodic(1, periodic_callback_1hz)

# Invoking a function once after a 0.5 second delay expires.
deferred_handle = node.defer(0.5, deferred_call)

# When we no longer want a callback to fire, the associated task can be removed.
periodic_handle.remove()

# Deferred calls get removed automatically once they were invoked.
# Removing a deferred call before its invocation can be used to cancel
# pending deferred calls if they are no longer needed.
deferred_handle.remove()
```

Note that all exceptions thrown from all kinds of callbacks from the node
are propagated through and thrown from the `spin()` method.
Make sure to catch them.

## Loading DSDL definitions

The library automatically loads definitions of all standard DSDL definitions
(from the namespace `uavcan.*`) immediately once the module is imported.
Additional definitions, such as vendor-specific definitions,
can be added later by invoking `uavcan.load_dsdl()` with a list of lookup paths provided in the first argument.
Refer to the function documentation for more info.

Standard definitions can be reached directly from the module namespace, e.g.
`uavcan.protocol.NodeStatus`.
Vendor-specific definitions can be accessed via the pseudo-module `uavcan.thirdparty`.
Both standard and vendor-specific types are also accessible via the global dictionaries
`uavcan.DATATYPES` and `uavcan.TYPENAMES`.

When instantiating a message type, its type name should be treated as a class name,
e.g. `msg = uavcan.protocol.NodeStatus()`.
When instantiating a service request or response structure,
its type name need to be appended with either `Request()` or `Response()`
specifier, e.g. `uavcan.protocol.GetNodeInfo.Request()`, `uavcan.protocol.GetNodeInfo.Response()`.
Direct instantiation of services is not defined by the specification,
and will lead to a runtime error.

## Receiving messages from the bus

```python
def node_status_callback(event):
    print('NodeStatus message from node', event.transfer.source_node_id)
    print('Node uptime:', event.message.uptime_sec, 'seconds')
    # Messages, service requests, service responses, and entire events
    # can be converted into YAML formatted data structure using to_yaml():
    print(uavcan.to_yaml(event))

# Subscribing to messages uavcan.protocol.NodeStatus
handle = node.add_handler(uavcan.protocol.NodeStatus, node_status_callback)

# When we no longer need them, we can remove the handler
handle.remove()
```

### Timestamping

It should be noted that Pyuavcan goes at great lengths to obtain precise timestamps
of the received CAN frames, and, by extension, UAVCAN transfers (both messages and services).
The exact method of timestamp estimation depends on the backend,
so the most valid information can be obtained by looking at the source code of the backend of interest.

For SLCAN, if the CAN adapter provides timestamps,
the library will convert them from the adapter's clock domain
into the monotonic and real time domains of the local computer using the Olson
passive time synchronization algorithm.
If timestamps are not provided by the adapter,
the library will use naive timestamping, which is imprecise.

For SocketCAN, the library retrieves the timestamping information
from the Linux kernel. Refer to the code for more information.

The computed precise timestamps will be stored in the fields
`event.transfer.ts_monotonic` and `event.transfer.ts_real` for time in
the local monotonic (see `time.monotonic()`) and real (see `time.time()`) domains, respectively.

## Publishing messages to the bus

Pyuavcan supports different of ways to assign values to fields of DSDL data structures.
Using the type constructors:

* `uavcan.protocol.debug.KeyValue(key='this is key', value=123.456)`
* `uavcan.protocol.param.GetSet.Request(name='foobar')`
* `uavcan.protocol.param.GetSet.Response(value=uavcan.protocol.param.Value(integer_value=123))`

{% include lightbox.html url="/Implementations/Pyuavcan/Tutorials/2._Basic_usage/gui_tool_bus_monitor_publishing.png" title="Broadcasts as seen from GUI Tool" thumbnail=true %}

Values that are not specified in the constructor are zero initialized by default.

Using direct assignment:

```python
s = uavcan.protocol.debug.KeyValue()
s.key = 'this is key'
s.value = 123.456
```

Messages can be broadcast using the method `broadcast()`:

```python
def do_publish():
    msg = uavcan.protocol.debug.KeyValue(key='this is key', value=123.456)
    # The priority argument can be omitted, in which case default will be used
    node.broadcast(msg, priority=uavcan.TRANSFER_PRIORITY_LOWEST)

handle = node.periodic(1, do_publish)
```

## Invoking services

```python
# The callback will be invoked ALWAYS, even if the request has timed out.
# In the case of a timeout, the argument will be None.
def response_callback(event):
    if event:
        print('Service response from server', event.transfer.source_node_id)
        print(uavcan.to_yaml(event.response))
    else:
        print('SERVICE REQUEST HAS TIMED OUT')

# The priority argument can be omitted, in which case default will be used
node.request(uavcan.protocol.GetNodeInfo.Request(), 127, response_callback,
             priority=uavcan.TRANSFER_PRIORITY_LOWEST)
node.request(uavcan.protocol.GetNodeInfo.Request(), 126, response_callback)
```

## Responding to services

{% include lightbox.html url="/Implementations/Pyuavcan/Tutorials/2._Basic_usage/gui_tool_restart.png" title="Sending restart requests from GUI Tool" thumbnail=true %}

```python
def handle_node_restart_request(event):
    print('Request from client', event.transfer.source_node_id)
    # Rejecting the request if the magic number is incorrect
    if event.request.magic_number != event.request.MAGIC_NUMBER:
        return uavcan.protocol.RestartNode.Response(ok=False)
    print('Restart request accepted')
    # Ha ha, just kidding; actual reboot in a demo application is hardly a good idea
    import os
    print('Reboot result:', os.system('reboot'))
    # Returning confirmation
    return uavcan.protocol.RestartNode.Response(ok=True)

node.add_handler(uavcan.protocol.RestartNode, handle_node_restart_request)
```

## Using Vendor-Specific DSDL Definitions

Pyuavcan will automatically scan the directory `~/uavcan_vendor_specific_types` for vendor-specific data types,
where `~` stands for the home directory of the current user, e.g. `/home/joe` or `C:\Users\Joe`.
Consider the following directory layout:

```
~
└── uavcan_vendor_specific_types
    └── sirius_cybernetics_corporation
        ├── 100.Foo.uavcan
        ├── 42.Bar.uavcan
        └── Baz.uavcan
```

The above layout defines the following custom data types:

* `sirius_cybernetics_corporation.Foo` with the default data type ID 100.
* `sirius_cybernetics_corporation.Bar` with the default data type ID 42.
* `sirius_cybernetics_corporation.Baz` where the default data type ID is not set.

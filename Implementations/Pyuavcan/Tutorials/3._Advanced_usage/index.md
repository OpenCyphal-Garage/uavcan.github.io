---
---

# Advanced usage

This article provides a brief overview of some of the more advanced features of Pyuavcan.
It does not aim to provide an exhaustive user guide; for that you should refer to the
Python's help system (the function `help()`) and to the source code,
particularly to the sub-package `uavcan.app`.

The examples below assume that the local node instance is bound to the variable `node`,
unless stated otherwise.

## Bus monitoring

IO hooks allow the application to monitor all incoming and outgoing CAN frames.
The Bus Monitor feature of the GUI Tool is based on this capability of Pyuavcan.

```python
def frame_hook(direction, frame):
    if direction == 'rx':
        print('<', frame)
    elif direction == 'tx':
        print('>', frame)
    else:
        raise Exception('Invalid direction specification: %r' % direction)

handle = node.can_driver.add_io_hook(frame_hook)

# The hook can be removed like that:
handle.remove()
```

## File server

File server can be used to perform firmware update on remote nodes,
as well as for general purpose file exchange.

```python
import uavcan

# File server need to be provided with file lookup pathes.
# These can be directories or files.
file_server = uavcan.app.file_server.FileServer(node, ['/home/pavel'])
file_server.lookup_paths.append('/etc/fstab')

print(file_server.lookup_paths)
print(file_server.path_hit_counters)

# The file server will be running in the background, requiring no additional attention
# When it is no longer needed, it should be finalized by calling close():
file_server.close()
```

You can test the file server using the following command in the Interactive Console of the GUI Tool:

```python
request(uavcan.protocol.file.Read.Request(path=uavcan.protocol.file.Path(path='/etc/fstab')), 123)
```

The file server uses the following logic for relative file path resolution (simplified).
It can be seen that locations specified earlier in the lookup path list take precedence.

```python
# This is simplified
def _try_resolve_relative_path(lookup_paths, rel_path):
    for p in lookup_paths:
        if p.endswith(rel_path) and os.path.isfile(p):
            return p
        if os.path.isfile(os.path.join(p, rel_path)):
            return os.path.join(p, rel_path)
```

## Dynamic node ID allocation

The logic pertaining to the dynamic node ID allocation feature is located in the module
`uavcan.app.dynamic_node_id`.
The code below demonstrates how to run an instance of a centralized node ID allocator.

```python
node_monitor = uavcan.app.node_monitor.NodeMonitor(node)
# It is NOT necessary to specify the database storage.
# If it is not specified, the allocation table will be kept in memory, thus it will not be persistent.
allocator = uavcan.app.dynamic_node_id.CentralizedServer(node, node_monitor,
                                                         database_storage='/home/pavel/allocation_table.db')

# The allocator and the node monitor will be running in the background, requiring no additional attention
# When they are no longer needed, they should be finalized by calling close():
allocator.close()
node_monitor.close()
```


#!/usr/bin/env python3

import uavcan, time, math


# Publishing setpoint values from this function; it is invoked periodically from the node thread.
def publish_throttle_setpoint():
    # Generating a sine wave
    setpoint = int(512 * (math.sin(time.time()) + 2))
    # Commanding ESC with indices 0, 1, 2, 3 only
    commands = [setpoint, setpoint, setpoint, setpoint]
    message = uavcan.equipment.esc.RawCommand(cmd=commands)
    node.broadcast(message)


if __name__ == '__main__':
    # Initializing a UAVCAN node instance.
    # In this example we're using an SLCAN adapter on the port '/dev/ttyACM0'.
    # PyUAVCAN also supports other types of adapters, refer to the docs to learn more.
    node = uavcan.make_node('/dev/ttyACM0', node_id=10, bitrate=1000000)

    # Initializing a dynamic node ID allocator.
    # This would not be necessary if the nodes were configured to use static node ID.
    node_monitor = uavcan.app.node_monitor.NodeMonitor(node)
    dynamic_node_id_allocator = uavcan.app.dynamic_node_id.CentralizedServer(node, node_monitor)

    # Waiting for at least one other node to appear online (our local node is already online).
    while len(dynamic_node_id_allocator.get_allocation_table()) <= 1:
        print('Waiting for other nodes to become online...')
        node.spin(timeout=1)

    # This is how we invoke the publishing function periodically.
    node.periodic(0.05, publish_throttle_setpoint)

    # Printing ESC status message to stdout in human-readable YAML format.
    node.add_handler(uavcan.equipment.esc.Status, lambda msg: print(uavcan.to_yaml(msg)))

    # Running the node until the application is terminated or until first error.
    try:
        node.spin()
    except KeyboardInterrupt:
        pass

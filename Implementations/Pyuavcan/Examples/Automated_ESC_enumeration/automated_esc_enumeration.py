#!/usr/bin/env python3

import uavcan, time


# Waiting until new nodes stop appearing online.
# That would mean that all nodes that are connected to the bus are now online and ready to work.
def wait_for_all_nodes_to_become_online():
    num_nodes = 0
    while True:
        node.spin(timeout=10)
        new_num_nodes = len(dynamic_node_id_allocator.get_allocation_table())
        if new_num_nodes == num_nodes and num_nodes > 1:
            break

        num_nodes = new_num_nodes


# Determining how many ESC nodes are present.
# In real use cases though the number of ESC should be obtained from elsewhere, e.g. from control mixer settings.
# There is a helper class in PyUAVCAN that allows one to automate what we're doing here,
# but we're not using it for the purposes of greater clarity of what's going on on the protocol level.
def detect_esc_nodes():
    esc_nodes = set()
    handle = node.add_handler(uavcan.equipment.esc.Status, lambda event: esc_nodes.add(event.transfer.source_node_id))
    try:
        node.spin(timeout=3)            # Collecting ESC status messages, thus determining which nodes are ESC
    finally:
        handle.remove()

    return esc_nodes


# Enumerating ESC.
# In this example we're using blocking code for simplicity reasons,
# but real applications will most likely resort either to asynchronous code (callback-based),
# or implement the logic in a dedicated thread.
# Conversion of the code from synchronous to asynchronous/multithreaded pertains to the domain of general
# programming issues, so these questions are not covered in this demo.
def enumerate_all_esc(esc_nodes, timeout=60):
    begin_responses_succeeded = 0

    def begin_response_checker(event):
        nonlocal begin_responses_succeeded
        if not event:
            raise Exception('Request timed out')

        if event.response.error != event.response.ERROR_OK:
            raise Exception('Enumeration rejected\n' + uavcan.to_yaml(event))

        begin_responses_succeeded += 1

    overall_deadline = time.monotonic() + timeout

    print('Starting enumeration on all nodes...')
    begin_request = uavcan.protocol.enumeration.Begin.Request(timeout_sec=timeout)
    for node_id in esc_nodes:
        print('Sending enumeration begin request to', node_id)
        node.request(begin_request, node_id, begin_response_checker)

    while begin_responses_succeeded < len(esc_nodes):
        node.spin(0.1)

    print('Listening for indications...')
    enumerated_nodes = []
    next_index = 0
    while set(enumerated_nodes) != esc_nodes:
        received_indication = None

        def indication_callback(event):
            nonlocal received_indication
            if event.transfer.source_node_id in enumerated_nodes:
                print('Indication callback from node %d ignored - already enumerated' % event.transfer.source_node_id)
            else:
                print(uavcan.to_yaml(event))
                received_indication = event

        indication_handler = node.add_handler(uavcan.protocol.enumeration.Indication, indication_callback)
        print('=== PROVIDE ENUMERATION FEEDBACK ON ESC INDEX %d NOW ===' % next_index)
        print('=== e.g. turn the motor, press the button, etc, depending on your equipment ===')
        try:
            while received_indication is None:
                node.spin(0.1)
                if time.monotonic() > overall_deadline:
                    raise Exception('Process timed out')
        finally:
            indication_handler.remove()

        target_node_id = received_indication.transfer.source_node_id
        print('Indication received from node', target_node_id)

        print('Stopping enumeration on node', target_node_id)
        begin_responses_succeeded = 0
        node.request(uavcan.protocol.enumeration.Begin.Request(), target_node_id, begin_response_checker)
        while begin_responses_succeeded < 1:
            node.spin(0.1)

        print('Setting config param %r to %r...' % (received_indication.message.parameter_name.decode(), next_index))
        configuration_finished = False

        def param_set_response(event):
            if not event:
                raise Exception('Request timed out')

            assert event.response.name == received_indication.message.parameter_name
            assert event.response.value.integer_value == next_index
            print(uavcan.to_yaml(event))
            node.request(uavcan.protocol.param.ExecuteOpcode.Request(
                             opcode=uavcan.protocol.param.ExecuteOpcode.Request().OPCODE_SAVE),
                         target_node_id,
                         param_opcode_response)

        def param_opcode_response(event):
            nonlocal configuration_finished
            if not event:
                raise Exception('Request timed out')

            print(uavcan.to_yaml(event))
            if not event.response.ok:
                raise Exception('Param opcode execution rejected\n' + uavcan.to_yaml(event))
            else:
                configuration_finished = True

        node.request(uavcan.protocol.param.GetSet.Request(value=uavcan.protocol.param.Value(integer_value=next_index),
                                                          name=received_indication.message.parameter_name),
                     target_node_id,
                     param_set_response)

        while not configuration_finished:
            node.spin(0.1)

        print('Node', target_node_id, 'assigned ESC index', next_index)
        next_index += 1
        enumerated_nodes.append(target_node_id)
        print('Enumerated so far:', enumerated_nodes)

    return enumerated_nodes


if __name__ == '__main__':
    # Initializing a UAVCAN node instance.
    # In this example we're using an SLCAN adapter on the port '/dev/ttyACM0'.
    # PyUAVCAN also supports other types of adapters, refer to its docs to learn more.
    node = uavcan.make_node('/dev/ttyACM0', node_id=10, bitrate=1000000)

    # Initializing a dynamic node ID allocator.
    # This would not be necessary if the nodes were configured to use static node ID.
    node_monitor = uavcan.app.node_monitor.NodeMonitor(node)
    dynamic_node_id_allocator = uavcan.app.dynamic_node_id.CentralizedServer(node, node_monitor)

    print('Waiting for all nodes to appear online, this should take less than a minute...')
    wait_for_all_nodes_to_become_online()
    print('Online nodes:', [node_id for _, node_id in dynamic_node_id_allocator.get_allocation_table()])

    print('Detecting ESC nodes...')
    esc_nodes = detect_esc_nodes()
    print('ESC nodes:', esc_nodes)

    enumerated_esc = enumerate_all_esc(esc_nodes)
    print('All ESC enumerated successfully; index order is as follows:', enumerated_esc)

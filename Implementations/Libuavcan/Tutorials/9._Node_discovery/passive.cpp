#include <iostream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/node_status_monitor.hpp>      // For uavcan::NodeStatusMonitor

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class implements a passive node monitor.
 * There's a basic node monitor implementation in the library: uavcan::NodeStatusMonitor
 * Extension through inheritance allows to add more complex logic to it.
 */
class NodeMonitor : public uavcan::NodeStatusMonitor
{
    /**
     * This method is not required to implement.
     * It is called when a remote node becomes online, changes status, or goes offline.
     */
    void handleNodeStatusChange(const NodeStatusChangeEvent& event) override
    {
        if (event.was_known)
        {
            std::cout << "Node " << int(event.node_id.get()) << " has changed status from "
                      << modeToString(event.old_status) << "/" << healthToString(event.old_status)
                      << " to "
                      << modeToString(event.status) << "/" << healthToString(event.status)
                      << std::endl;
        }
        else
        {
            std::cout << "Node " << int(event.node_id.get()) << " has just appeared with status "
                      << modeToString(event.status) << "/" << healthToString(event.status)
                      << std::endl;
        }
    }

    /**
     * This method is not required to implement.
     * It is called for every received message uavcan.protocol.NodeStatus after handleNodeStatusChange(), even
     * if the status code has not changed.
     */
    void handleNodeStatusMessage(const uavcan::ReceivedDataStructure<uavcan::protocol::NodeStatus>& msg) override
    {
        (void)msg;
        //std::cout << "Remote node status message\n" << msg << std::endl << std::endl;
    }

public:
    NodeMonitor(uavcan::INode& node) :
        uavcan::NodeStatusMonitor(node)
    { }

    static const char* modeToString(const NodeStatus status)
    {
        switch (status.mode)
        {
        case uavcan::protocol::NodeStatus::MODE_OPERATIONAL:     return "OPERATIONAL";
        case uavcan::protocol::NodeStatus::MODE_INITIALIZATION:  return "INITIALIZATION";
        case uavcan::protocol::NodeStatus::MODE_MAINTENANCE:     return "MAINTENANCE";
        case uavcan::protocol::NodeStatus::MODE_SOFTWARE_UPDATE: return "SOFTWARE_UPDATE";
        case uavcan::protocol::NodeStatus::MODE_OFFLINE:         return "OFFLINE";
        default: return "???";
        }
    }

    static const char* healthToString(const NodeStatus status)
    {
        switch (status.health)
        {
        case uavcan::protocol::NodeStatus::HEALTH_OK:       return "OK";
        case uavcan::protocol::NodeStatus::HEALTH_WARNING:  return "WARNING";
        case uavcan::protocol::NodeStatus::HEALTH_ERROR:    return "ERROR";
        case uavcan::protocol::NodeStatus::HEALTH_CRITICAL: return "CRITICAL";
        default: return "???";
        }
    }
};

int main()
{
    uavcan::Node<16384> node(getCanDriver(), getSystemClock());

    /*
     * In this example the node is configured in passive mode, i.e. without node ID.
     * This means that the node will not be able to emit transfers, which is not needed anyway.
     */
    node.setName("org.uavcan.tutorial.passive_monitor");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Instantiating the monitor.
     * The object is noncopyable.
     */
    NodeMonitor monitor(node);

    /*
     * Starting the monitor.
     * Once started, it runs in the background and does not require any attention.
     */
    const int monitor_start_res = monitor.start();
    if (monitor_start_res < 0)
    {
        throw std::runtime_error("Failed to start the monitor; error: " + std::to_string(monitor_start_res));
    }

    /*
     * Spinning the node for 2 seconds and then printing the list of nodes in the network.
     */
    if (node.spin(uavcan::MonotonicDuration::fromMSec(2000)) < 0)
    {
        throw std::runtime_error("Spin failed");
    }

    std::cout << "Known nodes:" << std::endl;
    for (int i = 1; i <= uavcan::NodeID::Max; i++)
    {
        if (monitor.isNodeKnown(i))
        {
            auto status = monitor.getNodeStatus(i);
            std::cout << "Node " << i << ": "
                      << NodeMonitor::modeToString(status) << "/" << NodeMonitor::healthToString(status)
                      << std::endl;
            /*
             * It is left as an exercise for the reader to call the following services for each discovered node:
             *  - uavcan.protocol.GetNodeInfo       - full node information (name, HW/SW version)
             *  - uavcan.protocol.GetTransportStats - transport layer statistics (num transfers, errors, iface stats)
             *  - uavcan.protocol.GetDataTypeInfo   - data type check: is supported? how used? is compatible?
             */
        }
    }

    /*
     * The monitor provides a method that finds first node with worst health.
     */
    if (monitor.findNodeWithWorstHealth().isUnicast())
    {
        /*
         * There's at least one node in the network.
         */
        auto status = monitor.getNodeStatus(monitor.findNodeWithWorstHealth());
        std::cout << "Worst node health: " << NodeMonitor::healthToString(status) << std::endl;
    }
    else
    {
        /*
         * The network is empty.
         */
        std::cout << "No other nodes in the network" << std::endl;
    }

    /*
     * Running the node.
     */
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }

    return 0;
}

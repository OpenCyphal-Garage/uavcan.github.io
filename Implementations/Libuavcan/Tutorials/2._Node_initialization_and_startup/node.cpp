#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/**
 * These functions are platform dependent, so they are not included in this example.
 * Refer to the relevant platform documentation to learn how to implement them.
 */
extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * Memory pool size largely depends on the number of CAN ifaces and on application's logic.
 * Please read the documentation for the class uavcan::Node to learn more.
 */
constexpr unsigned NodeMemoryPoolSize = 16384;

typedef uavcan::Node<NodeMemoryPoolSize> Node;

/**
 * Node object will be constructed at the time of the first access.
 * Note that most library objects are noncopyable (e.g. publishers, subscribers, servers, callers, timers, ...).
 * Attempt to copy a noncopyable object causes compilation failure.
 */
static Node& getNode()
{
    static Node node(getCanDriver(), getSystemClock());
    return node;
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    /*
     * Node initialization.
     * Node ID and name are required; otherwise, the node will refuse to start.
     * Version info is optional.
     */
    auto& node = getNode();

    node.setNodeID(self_node_id);

    node.setName("org.uavcan.tutorial.init");

    uavcan::protocol::SoftwareVersion sw_version;  // Standard type uavcan.protocol.SoftwareVersion
    sw_version.major = 1;
    node.setSoftwareVersion(sw_version);

    uavcan::protocol::HardwareVersion hw_version;  // Standard type uavcan.protocol.HardwareVersion
    hw_version.major = 1;
    node.setHardwareVersion(hw_version);

    /*
     * Start the node.
     * All returnable error codes are listed in the header file uavcan/error.hpp.
     */
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Informing other nodes that we're ready to work.
     * Default mode is INITIALIZING.
     */
    node.setModeOperational();

    /*
     * Some logging.
     * Log formatting is not available in C++03 mode.
     */
    node.getLogger().setLevel(uavcan::protocol::debug::LogLevel::DEBUG);
    node.logInfo("main", "Hello world! My Node ID: %*",
                 static_cast<int>(node.getNodeID().get()));

    std::cout << "Hello world!" << std::endl;

    /*
     * Node loop.
     * The thread should not block outside Node::spin().
     */
    while (true)
    {
        /*
         * If there's nothing to do, the thread blocks inside the driver's
         * method select() until the timeout expires or an error occurs (e.g. driver failure).
         * All error codes are listed in the header uavcan/error.hpp.
         */
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }

        /*
         * Random status transitions.
         * In real applications, the status code shall reflect node's health.
         */
        const float random = std::rand() / float(RAND_MAX);
        if (random < 0.7)
        {
            node.setHealthOk();
        }
        else if (random < 0.9)
        {
            node.setHealthWarning();
        }
        else
        {
            node.setHealthError();
        }
    }
}

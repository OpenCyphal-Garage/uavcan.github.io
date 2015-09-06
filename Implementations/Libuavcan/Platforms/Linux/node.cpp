#include <iostream>
#include <vector>
#include <string>
#include <uavcan_linux/uavcan_linux.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/param/ExecuteOpcode.hpp>

static uavcan_linux::NodePtr initNode(const std::vector<std::string>& ifaces, uavcan::NodeID nid,
                                      const std::string& name)
{
    auto node = uavcan_linux::makeNode(ifaces);

    node->setNodeID(nid);
    node->setName(name.c_str());

    {
        const auto app_id = uavcan_linux::makeApplicationID(uavcan_linux::MachineIDReader().read(), name, nid.get());

        uavcan::protocol::HardwareVersion hwver;
        std::copy(app_id.begin(), app_id.end(), hwver.unique_id.begin());
        std::cout << hwver << std::endl;

        node->setHardwareVersion(hwver);
    }

    /*
     * Starting the node
     */
    if (0 != node->start())
    {
        throw std::runtime_error("Bad luck");
    }

    return node;
}

static void runForever(const uavcan_linux::NodePtr& node)
{
    /*
     * Switching to the Operational mode to inform other nodes that we're ready to work now
     */
    node->setModeOperational();

    /*
     * Subscribing to the logging message just for fun
     */
    auto log_sub = node->makeSubscriber<uavcan::protocol::debug::LogMessage>(
        [](const uavcan::ReceivedDataStructure<uavcan::protocol::debug::LogMessage>& msg)
        {
            std::cout << msg << std::endl;
        });

    /*
     * Key Value publisher
     */
    auto keyvalue_pub = node->makePublisher<uavcan::protocol::debug::KeyValue>();

    /*
     * Timer that uses the above publisher once a minute
     */
    auto timer = node->makeTimer(uavcan::MonotonicDuration::fromMSec(60000), [&](const uavcan::TimerEvent&)
        {
            uavcan::protocol::debug::KeyValue msg;
            msg.key = "the_great_answer";
            msg.value = 42;
            (void)keyvalue_pub->broadcast(msg);
        });

    /*
     * A useless server that just prints the request and responds with a default-initialized response data structure
     */
    auto server = node->makeServiceServer<uavcan::protocol::param::ExecuteOpcode>(
        [](const uavcan::protocol::param::ExecuteOpcode::Request& req,
           uavcan::protocol::param::ExecuteOpcode::Response&)
        {
            std::cout << req << std::endl;
        });

    /*
     * Spinning forever
     */
    while (true)
    {
        const int res = node->spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            node->logError("spin", "Error %*", res);
        }
    }
}

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage:\n\t" << argv[0] << " <node-id> <can-iface-name-1> [can-iface-name-N...]" << std::endl;
        return 1;
    }
    const int self_node_id = std::stoi(argv[1]);
    std::vector<std::string> iface_names(argv + 2, argv + argc);
    auto node = initNode(iface_names, self_node_id, "org.uavcan.pan_galactic_gargle_blaster");
    std::cout << "Initialized" << std::endl;
    runForever(node);
    return 0;
}

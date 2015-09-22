#include <iostream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/file/BeginFirmwareUpdate.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

using uavcan::protocol::file::BeginFirmwareUpdate;

/**
 * This class demonstrates how to use uavcan::MethodBinder with service clients in C++03.
 * In C++11 and newer standards it is recommended to use lambdas and std::function<> instead, as this approach
 * would be much easier to implement and to understand.
 */
class Node
{
    static const unsigned NodeMemoryPoolSize = 16384;

    uavcan::Node<NodeMemoryPoolSize> node_;

    /*
     * Instantiation of MethodBinder
     */
    typedef uavcan::MethodBinder<Node*, void (Node::*)(const uavcan::ServiceCallResult<BeginFirmwareUpdate>&) const>
        BeginFirmwareUpdateCallbackBinder;

    void beginFirmwareUpdateCallback(const uavcan::ServiceCallResult<BeginFirmwareUpdate>& res) const
    {
        if (res.isSuccessful())
        {
            std::cout << res << std::endl;
        }
        else
        {
            std::cerr << "Service call to node "
                      << static_cast<int>(res.getCallID().server_node_id.get())
                      << " has failed" << std::endl;
        }
    }

public:
    Node(uavcan::NodeID self_node_id, const std::string& self_node_name) :
        node_(getCanDriver(), getSystemClock())
    {
        node_.setNodeID(self_node_id);
        node_.setName(self_node_name.c_str());
    }

    void start()
    {
        const int start_res = node_.start();
        if (start_res < 0)
        {
            throw std::runtime_error("Failed to start the node: " + std::to_string(start_res));
        }
    }

    void execute(uavcan::NodeID server_node_id)
    {
        /*
         * Initializing the request structure
         */
        BeginFirmwareUpdate::Request request;
        request.image_file_remote_path.path = "/foo/bar";

        /*
         * Initializing the client object
         */
        uavcan::ServiceClient<BeginFirmwareUpdate, BeginFirmwareUpdateCallbackBinder> client(node_);

        client.setCallback(BeginFirmwareUpdateCallbackBinder(this, &Node::beginFirmwareUpdateCallback));

        /*
         * Calling
         */
        const int call_res = client.call(server_node_id, request);
        if (call_res < 0)
        {
            throw std::runtime_error("Unable to perform service call: " + std::to_string(call_res));
        }

        /*
         * Spinning the node until the call is completed
         */
        node_.setModeOperational();
        while (client.hasPendingCalls())  // Whether the call has completed (doesn't matter successfully or not)
        {
            const int res = node_.spin(uavcan::MonotonicDuration::fromMSec(10));
            if (res < 0)
            {
                std::cerr << "Transient failure: " << res << std::endl;
            }
        }
    }
};

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id> <server-node-id>" << std::endl;
        return 1;
    }

    const uavcan::NodeID self_node_id = std::stoi(argv[1]);
    const uavcan::NodeID server_node_id = std::stoi(argv[2]);

    Node node(self_node_id, "org.uavcan.tutorial.clientcpp03");

    node.start();

    node.execute(server_node_id);

    return 0;
}

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * This example uses the service type uavcan.protocol.file.BeginFirmwareUpdate.
 */
#include <uavcan/protocol/file/BeginFirmwareUpdate.hpp>


extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

constexpr unsigned NodeMemoryPoolSize = 16384;
typedef uavcan::Node<NodeMemoryPoolSize> Node;

static Node& getNode()
{
    static Node node(getCanDriver(), getSystemClock());
    return node;
}

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::printf("Usage: %s <self-node-id> <server-node-id>\n", argv[0]);
        return 1;
    }

    const uavcan::NodeID self_node_id = std::stoi(argv[1]);
    const uavcan::NodeID server_node_id = std::stoi(argv[2]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.client");

    while (true)
    {
        const int res = node.start();
        if (res < 0)
        {
            std::printf("Node start failed: %d, will retry\n", res);
            ::sleep(1);
        }
        else { break; }
    }

    /*
     * Initializing the client. Remember that client objects are noncopyable.
     * Note that calling client.init() is not necessary - the object can be initialized ad hoc during the first call.
     */
    using uavcan::protocol::file::BeginFirmwareUpdate;
    uavcan::ServiceClient<BeginFirmwareUpdate> client(node);

    const int client_init_res = client.init();
    if (client_init_res < 0)
    {
        std::exit(1);                   // TODO proper error handling
    }

    /*
     * Setting the callback.
     * This callback will be executed when the call is completed.
     * Note that the callback will ALWAYS be called even if the service call has timed out; this guarantee
     * allows to simplify error handling in the application.
     */
    client.setCallback([](const uavcan::ServiceCallResult<BeginFirmwareUpdate>& call_result)
        {
            if (call_result.isSuccessful())  // Whether the call was successful, i.e. whether the response was received
            {
                std::cout << call_result << std::endl;
            }
            else
            {
                std::cout << "Service call to node "
                          << static_cast<int>(call_result.getCallID().server_node_id.get())
                          << " has failed" << std::endl;
            }
        });

    /*
     * Service call timeout can be overridden if needed, though it's not recommended.
     */
    client.setRequestTimeout(uavcan::MonotonicDuration::fromMSec(200));

    /*
     * Calling the remote service.
     * Generated service data types have two nested types:
     *   T::Request  - request data structure
     *   T::Response - response data structure
     * For the service data structure, it is not possible to instantiate T itself, nor does it make any sense.
     */
    BeginFirmwareUpdate::Request request;
    request.image_file_remote_path.path = "/foo/bar";

    const int call_res = client.call(server_node_id, request);
    if (call_res < 0)
    {
        std::printf("Unable to perform service call: %d\n", call_res);
        std::exit(1);
    }

    /*
     * Spin until the call is completed, then exit.
     */
    node.setModeOperational();
    while (client.hasPendingCalls())  // Whether the call has completed (doesn't matter successfully or not)
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(10));
        if (res < 0)
        {
            std::printf("Transient failure: %d\n", res);
        }
    }

    std::printf("Exit\n");
    return 0;
}

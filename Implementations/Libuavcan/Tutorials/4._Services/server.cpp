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
    if (argc < 2)
    {
        std::printf("Usage: %s <node-id>\n", argv[0]);
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.server");

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
     * Starting the server.
     * This server doesn't do anything useful; it just prints the received request and returns some meaningless
     * response.
     *
     * The service callback accepts two arguments:
     *  - a reference to a request structure (input)
     *  - a reference to a default-initialized response structure (output)
     * The type of the input can be either of these two:
     *  - T::Request&
     *  - uavcan::ReceivedDataStructure<T::Request>&
     * The type of the output is strictly T::Response&.
     * Note that for the service data structure, it is not possible to instantiate T itself, nor does it make any
     * sense.
     *
     * In C++11 mode, callback type defaults to std::function<>.
     * In C++03 mode, callback type defaults to a plain function pointer; use a binder object to call member
     * functions as callbacks (refer to uavcan::MethodBinder<>).
     */
    using uavcan::protocol::file::BeginFirmwareUpdate;
    uavcan::ServiceServer<BeginFirmwareUpdate> srv(node);

    const int srv_start_res = srv.start(
        [&](const uavcan::ReceivedDataStructure<BeginFirmwareUpdate::Request>& req, BeginFirmwareUpdate::Response& rsp)
        {
            std::cout << req << std::endl;
            rsp.error = rsp.ERROR_UNKNOWN;
            rsp.optional_error_message = "Our sun is dying";
        });

    if (srv_start_res < 0)
    {
        std::exit(1);                   // TODO proper error handling
    }

    /*
     * Node loop.
     */
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (res < 0)
        {
            std::printf("Transient failure: %d\n", res);
        }
    }
}

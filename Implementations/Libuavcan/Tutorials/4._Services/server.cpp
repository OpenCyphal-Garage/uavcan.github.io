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
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.server");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
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
    /*
     * C++03 WARNING
     * The code above will not compile in C++03, because it uses a lambda function.
     * In order to compile the code in C++03, move the code from the lambda to a standalone static function.
     * Use uavcan::MethodBinder<> to invoke member functions.
     */

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

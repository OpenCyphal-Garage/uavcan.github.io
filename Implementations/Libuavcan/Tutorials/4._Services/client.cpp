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
        std::cerr << "Usage: " << argv[0] << " <node-id> <server-node-id>" << std::endl;
        return 1;
    }

    const uavcan::NodeID self_node_id = std::stoi(argv[1]);
    const uavcan::NodeID server_node_id = std::stoi(argv[2]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.client");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
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
        throw std::runtime_error("Failed to init the client; error: " + std::to_string(client_init_res));
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
                // The result can be directly streamed; the output will be formatted in human-readable YAML.
                std::cout << call_result << std::endl;
            }
            else
            {
                std::cerr << "Service call to node "
                          << static_cast<int>(call_result.getCallID().server_node_id.get())
                          << " has failed" << std::endl;
            }
        });
    /*
     * C++03 WARNING
     * The code above will not compile in C++03, because it uses a lambda function.
     * In order to compile the code in C++03, move the code from the lambda to a standalone static function.
     * Use uavcan::MethodBinder<> to invoke member functions.
     */

    /*
     * Service call timeout can be overridden if needed, though it's not recommended.
     */
    client.setRequestTimeout(uavcan::MonotonicDuration::fromMSec(200));

    /*
     * It is possible to adjust priority of the outgoing service request transfers.
     * According to the specification, the services are supposed to use the same priority for response transfers.
     * Default priority is medium, which is 16.
     */
    client.setPriority(uavcan::TransferPriority::OneHigherThanLowest);

    /*
     * Calling the remote service.
     * Generated service data types have two nested types:
     *   T::Request  - request data structure
     *   T::Response - response data structure
     * For the service data structure, it is not possible to instantiate T itself, nor does it make any sense.
     */
    BeginFirmwareUpdate::Request request;
    request.image_file_remote_path.path = "/foo/bar";

    /*
     * It is possible to perform multiple concurrent calls using the same client object.
     * The class ServiceClient provides the following methods that allow to control execution of each call:
     *
     *  int call(NodeID server_node_id, const RequestType& request)
     *      Initiate a new non-blocking call.
     *
     *  int call(NodeID server_node_id, const RequestType& request, ServiceCallID& out_call_id)
     *      Initiate a new non-blocking call and return its ServiceCallID descriptor by reference.
     *      The descriptor allows to query the progress of the call or cancel it later.
     *
     *  void cancelCall(ServiceCallID call_id)
     *      Cancel a specific call using its descriptor.
     *
     *  void cancelAllCalls()
     *      Cancel all calls.
     *
     *  bool hasPendingCallToServer(NodeID server_node_id) const
     *      Whether the client object has pending calls to the given server at the moment.
     *
     *  unsigned getNumPendingCalls() const
     *      Returns the total number of pending calls at the moment.
     *
     *  bool hasPendingCalls() const
     *      Whether the client object has any pending calls at the moment.
     */
    const int call_res = client.call(server_node_id, request);
    if (call_res < 0)
    {
        throw std::runtime_error("Unable to perform service call: " + std::to_string(call_res));
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
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }

    return 0;
}

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

#include <uavcan/protocol/file/BeginFirmwareUpdate.hpp>
#include <uavcan/equipment/air_data/Sideslip.hpp>
#include <uavcan/equipment/air_data/TrueAirspeed.hpp>

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

    /*
     * Initializing a new node. Refer to the "Node initialization and startup" tutorial for more details.
     */
    const int self_node_id = std::stoi(argv[1]);
    const uavcan::NodeID server_node_id = std::stoi(argv[2]);
    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.publisher");
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing publishers. Please refer to the "Publishers and subscribers" tutorial for more details.
     */
    uavcan::Publisher<uavcan::equipment::air_data::Sideslip> sideslip_pub(node);
    const int sideslip_pub_init_res = sideslip_pub.init();
    if (sideslip_pub_init_res < 0)
    {
        throw std::runtime_error("Failed to start the publisher; error: " + std::to_string(sideslip_pub_init_res));
    }

    uavcan::Publisher<uavcan::equipment::air_data::TrueAirspeed> airspeed_pub(node);
    const int airspeed_pub_init_res = airspeed_pub.init();
    if (airspeed_pub_init_res < 0)
    {
        throw std::runtime_error("Failed to start the publisher; error: " + std::to_string(airspeed_pub_init_res));
    }

    /*
     * Initializing service client. Please refer to the "Services" tutorial for more details.
     */
    using uavcan::protocol::file::BeginFirmwareUpdate;
    uavcan::ServiceClient<BeginFirmwareUpdate> client(node);
    const int client_init_res = client.init();
    if (client_init_res < 0)
    {
        throw std::runtime_error("Failed to init the client; error: " + std::to_string(client_init_res));
    }
    client.setCallback([](const uavcan::ServiceCallResult<BeginFirmwareUpdate>& call_result)
    {
        if (call_result.isSuccessful())
        {
            std::cout << call_result << std::endl;
        }
        else
        {
            std::cerr << "Service call to node "
                      << static_cast<int>(call_result.getCallID().server_node_id.get())
                      << " has failed" << std::endl;
        }
    });
    client.setRequestTimeout(uavcan::MonotonicDuration::fromMSec(200));

    node.setModeOperational();

    while (true)
    {
        /*
         * Constantly publishing messages and sending requests to the server.
         */
        const int spin_res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (spin_res < 0)
        {
            std::cerr << "Transient failure: " << spin_res << std::endl;
        }

        uavcan::equipment::air_data::Sideslip sideslip_msg;
        sideslip_msg.sideslip_angle = std::rand() / float(RAND_MAX);
        sideslip_msg.sideslip_angle_variance = std::rand() / float(RAND_MAX);
        const int sideslip_msg_pub_res = sideslip_pub.broadcast(sideslip_msg);
        if (sideslip_msg_pub_res < 0)
        {
            std::cerr << "KV publication failure: " << sideslip_msg_pub_res << std::endl;
        }

        uavcan::equipment::air_data::TrueAirspeed airspd_msg;
        airspd_msg.true_airspeed = 10;
        airspd_msg.true_airspeed_variance = 1;
        const int airspd_msg_pub_res = airspeed_pub.broadcast(airspd_msg);
        if (airspd_msg_pub_res < 0)
        {
            std::cerr << "KV publication failure: " << airspd_msg_pub_res << std::endl;
        }

        BeginFirmwareUpdate::Request request;
        request.image_file_remote_path.path = "/foo/bar";
        const int call_res = client.call(server_node_id, request);
        if (call_res < 0)
        {
            throw std::runtime_error("Unable to perform service call: " + std::to_string(call_res));
        }
    }
}

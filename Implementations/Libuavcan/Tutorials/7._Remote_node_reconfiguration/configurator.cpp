#include <iostream>
#include <unistd.h>
#include <vector>
#include <uavcan/uavcan.hpp>

/*
 * Remote reconfiguration services.
 */
#include <uavcan/protocol/param/GetSet.hpp>
#include <uavcan/protocol/param/ExecuteOpcode.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

constexpr unsigned NodeMemoryPoolSize = 16384;

/**
 * Convenience function for blocking service calls.
 */
template <typename T>
std::pair<int, typename T::Response> performBlockingServiceCall(uavcan::INode& node,
                                                                uavcan::NodeID remote_node_id,
                                                                const typename T::Request& request)
{
    bool success = false;
    typename T::Response response;  // Generated types have zero-initializing constructors, always.

    uavcan::ServiceClient<T> client(node);
    client.setCallback([&](const uavcan::ServiceCallResult<T>& result)
        {
            success = result.isSuccessful();
            response = result.getResponse();
        });

    const int call_res = client.call(remote_node_id, request);
    if (call_res >= 0)
    {
        while (client.hasPendingCalls())
        {
            const int spin_res = node.spin(uavcan::MonotonicDuration::fromMSec(2));
            if (spin_res < 0)
            {
                return {spin_res, response};
            }
        }
        return {success ? 0 : -uavcan::ErrFailure, response};
    }
    return {call_res, response};
}

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id> <remote-node-id>" << std::endl;
        return 1;
    }

    const uavcan::NodeID self_node_id = std::stoi(argv[1]);
    const uavcan::NodeID remote_node_id = std::stoi(argv[2]);

    uavcan::Node<NodeMemoryPoolSize> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.configurator");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    node.setModeOperational();

    /*
     * Reading all params from the remote node (access by index); printing request/response
     * to stdout in YAML format.
     * Note that access by index should be used only to list params, not to get or set them.
     */
    std::vector<uavcan::protocol::param::GetSet::Response> remote_params;

    while (true)
    {
        uavcan::protocol::param::GetSet::Request req;
        req.index = remote_params.size();
        std::cout << "Param GET request:\n" << req << std::endl << std::endl;
        auto res = performBlockingServiceCall<uavcan::protocol::param::GetSet>(node, remote_node_id, req);
        if (res.first < 0)
        {
            throw std::runtime_error("Failed to get param: " + std::to_string(res.first));
        }
        if (res.second.name.empty())  // Empty name means no such param, which means we're finished
        {
            std::cout << "Param read done!\n\n" << std::endl;
            break;
        }
        std::cout << "Response:\n" << res.second << std::endl << std::endl;
        remote_params.push_back(res.second);
    }

    /*
     * Setting all parameters to their maximum values, if applicable. Access by name.
     */
    for (auto p : remote_params)
    {
        if (p.max_value.is(uavcan::protocol::param::NumericValue::Tag::empty))
        {
            std::cout << "Maximum value for parameter '" << p.name.c_str() << "' is not defined." << std::endl;
            continue;
        }

        uavcan::protocol::param::GetSet::Request req;
        req.name = p.name;

        if (p.max_value.is(uavcan::protocol::param::NumericValue::Tag::integer_value))
        {
            req.value.to<uavcan::protocol::param::Value::Tag::integer_value>() =
                p.max_value.to<uavcan::protocol::param::NumericValue::Tag::integer_value>();
        }
        else if (p.max_value.is(uavcan::protocol::param::NumericValue::Tag::real_value))
        {
            req.value.to<uavcan::protocol::param::Value::Tag::real_value>() =
                p.max_value.to<uavcan::protocol::param::NumericValue::Tag::real_value>();
        }
        else
        {
            throw std::runtime_error("Unknown tag in NumericValue: " + std::to_string(p.max_value.getTag()));
        }

        std::cout << "Param SET request:\n" << req << std::endl << std::endl;
        auto res = performBlockingServiceCall<uavcan::protocol::param::GetSet>(node, remote_node_id, req);
        if (res.first < 0)
        {
            throw std::runtime_error("Failed to set param: " + std::to_string(res.first));
        }
        std::cout << "Response:\n" << res.second << std::endl << std::endl;
    }
    std::cout << "Param set done!\n\n" << std::endl;

    /*
     * Reset all params back to their default values.
     */
    uavcan::protocol::param::ExecuteOpcode::Request opcode_req;
    opcode_req.opcode = opcode_req.OPCODE_ERASE;
    auto erase_res =
        performBlockingServiceCall<uavcan::protocol::param::ExecuteOpcode>(node, remote_node_id, opcode_req);
    if (erase_res.first < 0)
    {
        throw std::runtime_error("Failed to erase params: " + std::to_string(erase_res.first));
    }
    std::cout << "Param erase response:\n" << erase_res.second << std::endl << std::endl;
    std::cout << "Param erase done!\n\n" << std::endl;

    /*
     * Restart the remote node.
     */
    uavcan::protocol::RestartNode::Request restart_req;
    restart_req.magic_number = restart_req.MAGIC_NUMBER;
    auto restart_res = performBlockingServiceCall<uavcan::protocol::RestartNode>(node, remote_node_id, restart_req);
    if (restart_res.first < 0)
    {
        throw std::runtime_error("Failed to restart: " + std::to_string(restart_res.first));
    }
    std::cout << "Restart response:\n" << restart_res.second << std::endl << std::endl;
    std::cout << "Restart done!" << std::endl;

    return 0;
}

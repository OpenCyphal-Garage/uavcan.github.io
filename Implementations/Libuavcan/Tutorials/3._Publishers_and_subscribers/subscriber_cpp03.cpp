#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/debug/LogMessage.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class demonstrates how to use uavcan::MethodBinder with subscriber objects in C++03.
 * In C++11 and newer standards it is recommended to use lambdas and std::function<> instead, as this approach
 * would be much easier to implement and to understand.
 */
class Node
{
    static const unsigned NodeMemoryPoolSize = 16384;

    uavcan::Node<NodeMemoryPoolSize> node_;

    /*
     * Instantiations of uavcan::MethodBinder<>
     */
    typedef uavcan::MethodBinder<Node*, void (Node::*)(const uavcan::protocol::debug::LogMessage&) const>
        LogMessageCallbackBinder;

    typedef uavcan::MethodBinder<Node*,
        void (Node::*)(const uavcan::ReceivedDataStructure<uavcan::protocol::debug::KeyValue>&) const>
            KeyValueCallbackBinder;

    uavcan::Subscriber<uavcan::protocol::debug::LogMessage, LogMessageCallbackBinder> log_sub_;
    uavcan::Subscriber<uavcan::protocol::debug::KeyValue, KeyValueCallbackBinder> kv_sub_;

    void logMessageCallback(const uavcan::protocol::debug::LogMessage& msg) const
    {
        std::cout << "Log message:\n" << msg << std::endl;
    }

    void keyValueCallback(const uavcan::ReceivedDataStructure<uavcan::protocol::debug::KeyValue>& msg) const
    {
        std::cout << "KV message:\n" << msg << std::endl;
    }

public:
    Node(uavcan::NodeID self_node_id, const std::string& self_node_name) :
        node_(getCanDriver(), getSystemClock()),
        log_sub_(node_),
        kv_sub_(node_)
    {
        node_.setNodeID(self_node_id);
        node_.setName(self_node_name.c_str());
    }

    void run()
    {
        const int start_res = node_.start();
        if (start_res < 0)
        {
            throw std::runtime_error("Failed to start the node: " + std::to_string(start_res));
        }

        const int log_sub_start_res = log_sub_.start(LogMessageCallbackBinder(this, &Node::logMessageCallback));
        if (log_sub_start_res < 0)
        {
            throw std::runtime_error("Failed to start the log subscriber; error: " + std::to_string(log_sub_start_res));
        }

        const int kv_sub_start_res = kv_sub_.start(KeyValueCallbackBinder(this, &Node::keyValueCallback));
        if (kv_sub_start_res < 0)
        {
            throw std::runtime_error("Failed to start the KV subscriber; error: " + std::to_string(kv_sub_start_res));
        }

        node_.setModeOperational();

        while (true)
        {
            const int res = node_.spin(uavcan::MonotonicDuration::getInfinite());
            if (res < 0)
            {
                std::cerr << "Transient failure: " << res << std::endl;
            }
        }
    }
};

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    Node node(self_node_id, "org.uavcan.tutorial.subscriber_cpp03");

    node.run();
}

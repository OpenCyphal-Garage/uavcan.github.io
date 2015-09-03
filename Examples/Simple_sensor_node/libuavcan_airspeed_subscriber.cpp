/*
 * This program subscribes to airspeed messages using libuavcan, and prints them into stdout in YAML format.
 * It can be used to test alternative implementations of the UAVCAN stack against the reference implementation.
 *
 * GCC invocation command:
 *     g++ libuavcan_airspeed_subscriber.cpp -std=c++11 -lrt -luavcan
 *
 * Author: Pavel Kirienko <pavel.kirienko@zubax.com>
 * License: CC0, no copyright reserved
 * Language: C++11
 */

#include <iostream>
#include <algorithm>
#include <uavcan_linux/uavcan_linux.hpp>
#include <uavcan/equipment/air_data/TrueAirspeed.hpp>

uavcan_linux::NodePtr initNode(const std::vector<std::string>& ifaces, uavcan::NodeID nid, const std::string& name)
{
    auto node = uavcan_linux::makeNode(ifaces);
    node->setNodeID(nid);
    node->setName(name.c_str());

    if (node->start() < 0)
    {
        throw std::runtime_error("Failed to start UAVCAN node");
    }

    node->setModeOperational();
    return node;
}

template <typename DataType>
static void printMessage(const uavcan::ReceivedDataStructure<DataType>& msg)
{
    std::cout << "[" << DataType::getDataTypeFullName() << "]\n" << msg << "\n---" << std::endl;
}

template <typename DataType>
static std::shared_ptr<uavcan::Subscriber<DataType>> makePrintingSubscriber(const uavcan_linux::NodePtr& node)
{
    return node->makeSubscriber<DataType>(&printMessage<DataType>);
}

static void runForever(const uavcan_linux::NodePtr& node)
{
    auto sub_true_airspeed = makePrintingSubscriber<uavcan::equipment::air_data::TrueAirspeed>(node);

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
    uavcan_linux::NodePtr node = initNode(iface_names, self_node_id, "org.uavcan.example.airspeed_subscriber");
    runForever(node);
    return 0;
}

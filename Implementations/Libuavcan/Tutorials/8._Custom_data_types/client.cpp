#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * The custom data types.
 */
#include <sirius_cybernetics_corporation/GetCurrentTime.hpp>
#include <sirius_cybernetics_corporation/PerformLinearLeastSquaresFit.hpp>

using sirius_cybernetics_corporation::GetCurrentTime;
using sirius_cybernetics_corporation::PerformLinearLeastSquaresFit;

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

constexpr unsigned NodeMemoryPoolSize = 16384;

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
    node.setName("org.uavcan.tutorial.custom_dsdl_client");

    /*
     * Configuring the Data Type IDs.
     * See the server sources for details.
     */
    auto regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<PerformLinearLeastSquaresFit>(243);
    if (regist_result != uavcan::GlobalDataTypeRegistry::RegistrationResultOk)
    {
        throw std::runtime_error("Failed to register the data type: " + std::to_string(regist_result));
    }

    regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<GetCurrentTime>(211);
    if (regist_result != uavcan::GlobalDataTypeRegistry::RegistrationResultOk)
    {
        throw std::runtime_error("Failed to register the data type: " + std::to_string(regist_result));
    }

    /*
     * Starting the node
     */
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Calling both services once; the result will be printed to stdout as YAML.
     */
    uavcan::ServiceClient<GetCurrentTime> cln_time(node);
    cln_time.setCallback([](const uavcan::ServiceCallResult<GetCurrentTime>& res)
        {
            std::cout << res << std::endl;
        });
    int res = cln_time.call(remote_node_id, GetCurrentTime::Request());
    if (res < 0)
    {
        throw std::runtime_error("Failed to call GetCurrentTime: " + std::to_string(res));
    }

    uavcan::ServiceClient<PerformLinearLeastSquaresFit> cln_least_squares(node);
    cln_least_squares.setCallback([](const uavcan::ServiceCallResult<PerformLinearLeastSquaresFit>& res)
        {
            std::cout << res << std::endl;
        });
    PerformLinearLeastSquaresFit::Request request;
    for (unsigned i = 0; i < 30; i++)
    {
        sirius_cybernetics_corporation::PointXY p;
        p.x = i * 2.5 + 10;
        p.y = i;
        request.points.push_back(p);
    }
    res = cln_least_squares.call(remote_node_id, request);
    if (res < 0)
    {
        throw std::runtime_error("Failed to call PerformLinearLeastSquaresFit: " + std::to_string(res));
    }

    /*
     * Spinning the node until both calls are finished.
     */
    node.setModeOperational();
    while (cln_time.hasPendingCalls() && cln_least_squares.hasPendingCalls())
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(10));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

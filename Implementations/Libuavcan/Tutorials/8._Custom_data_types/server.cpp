#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

#include <uavcan/protocol/debug/LogMessage.hpp>      // For purposes of example; not actually necessary.

/*
 * The custom data types.
 * If a data type has a default Data Type ID, it will be registered automatically once included
 * (the registration will be done before main() from a static constructor).
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
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const uavcan::NodeID self_node_id = std::stoi(argv[1]);

    uavcan::Node<NodeMemoryPoolSize> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.custom_dsdl_server");

    /*
     * We defined two data types, but only one of them has a default Data Type ID (DTID):
     *  - sirius_cybernetics_corporation.GetCurrentTime               - default DTID 242
     *  - sirius_cybernetics_corporation.PerformLinearLeastSquaresFit - default DTID is not set
     * The first one can be used as is; the second one needs to be registered first.
     */
    auto regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<PerformLinearLeastSquaresFit>(243); // DTID = 243

    if (regist_result != uavcan::GlobalDataTypeRegistry::RegistrationResultOk)
    {
        /*
         * Possible reasons for a failure:
         * - Data type name or ID is not unique
         * - Data Type Registry has been frozen and can't be modified anymore
         */
        throw std::runtime_error("Failed to register the data type: " + std::to_string(regist_result));
    }

    /*
     * Now we can use both data types:
     *  - sirius_cybernetics_corporation.GetCurrentTime               - DTID 242
     *  - sirius_cybernetics_corporation.PerformLinearLeastSquaresFit - DTID 243
     *
     * But here's more:
     * The specification requires that "the end user must be able to change the ID of any non-standard data type".
     * So assume that the end user needs to change the default value of 242 to 211. No problem, as long as the
     * Data Type Registry is not frozen. Once frozen, the registry can't alter the established data type configuration
     * anymore.
     *
     * Note that non-default Data Type IDs should normally be kept as node configuration params to make them easily
     * accessible for the user. Please refer to the relevant tutorial to learn how to make the node configuration
     * accessible via UAVCAN.
     * Also, this part of the specification is highly relevant - it describes parameter naming conventions for DTID:
     *  https://uavcan.org/Specification/6._Application_level_functions/#node-configuration
     */
    regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<GetCurrentTime>(211);  // DTID: 242 --> 211

    if (regist_result != uavcan::GlobalDataTypeRegistry::RegistrationResultOk)
    {
        throw std::runtime_error("Failed to register the data type: " + std::to_string(regist_result));
    }

    /*
     * The DTID of standard types can be changed also.
     */
    regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<uavcan::protocol::debug::LogMessage>(20999);

    if (regist_result != uavcan::GlobalDataTypeRegistry::RegistrationResultOk)
    {
        throw std::runtime_error("Failed to register the data type: " + std::to_string(regist_result));
    }

    /*
     * The current configuration is as follows:
     *  - sirius_cybernetics_corporation.GetCurrentTime                - DTID 211
     *  - sirius_cybernetics_corporation.PerformLinearLeastSquaresFit  - DTID 243
     *  - uavcan.protocol.debug.LogMessage                             - DTID 20999
     * Let's check it.
     * We can query data type info either by full name or by data type ID using the method find().
     * If there's no such type, find() returns nullptr.
     */
    auto descriptor = uavcan::GlobalDataTypeRegistry::instance().find("sirius_cybernetics_corporation.GetCurrentTime");
    assert(descriptor != nullptr);
    assert(descriptor->getID() == uavcan::DataTypeID(211));
    assert(descriptor->getID() != GetCurrentTime::DefaultDataTypeID); // There's a T::DefaultDataTypeID if the default DTID is set

    descriptor = uavcan::GlobalDataTypeRegistry::instance().find(uavcan::DataTypeKindService, uavcan::DataTypeID(243));
    assert(descriptor != nullptr);
    assert(std::string(descriptor->getFullName()) == "sirius_cybernetics_corporation.PerformLinearLeastSquaresFit");

    descriptor = uavcan::GlobalDataTypeRegistry::instance().find("uavcan.protocol.debug.LogMessage");
    assert(descriptor != nullptr);
    assert(descriptor->getID() == uavcan::DataTypeID(20999));

    /*
     * Starting the node as usual.
     * The Data Type Registry will be frozen once the node is started, then it can't be unfrozen again.
     */
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Warning: you can't change the data type configuration once the node is started because the Data Type Registry
     * is frozen now.
     */
    assert(uavcan::GlobalDataTypeRegistry::instance().isFrozen()); // It is frozen indeed.

    /*
     * Don't try this at home.
     */
    regist_result =
        uavcan::GlobalDataTypeRegistry::instance().registerDataType<GetCurrentTime>(GetCurrentTime::DefaultDataTypeID);
    assert(regist_result == uavcan::GlobalDataTypeRegistry::RegistrationResultFrozen);   // Will fail.

    /*
     * Now we can start the services.
     * There's nothing unusual at all.
     */
    uavcan::ServiceServer<GetCurrentTime> srv_get_current_time(node);
    int res = srv_get_current_time.start(
        [&](const GetCurrentTime::Request& request, GetCurrentTime::Response& response)
        {
            (void)request; // It's empty
            response.time = node.getUtcTime();  // Note: uavcan::UtcTime implicitly converts to uavcan.Timestamp!
        });
    if (res < 0)
    {
        throw std::runtime_error("Failed to start the GetCurrentTime server: " + std::to_string(res));
    }

    uavcan::ServiceServer<PerformLinearLeastSquaresFit> srv_least_squares(node);
    res = srv_least_squares.start(
        [](const PerformLinearLeastSquaresFit::Request& request, PerformLinearLeastSquaresFit::Response& response)
        {
            double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
            for (auto point : request.points)
            {
                sum_x += point.x;
                sum_y += point.y;
                sum_xy += point.x * point.y;
                sum_xx += point.x * point.x;
            }
            const double a = sum_x * sum_y - request.points.size() * sum_xy;
            const double b = sum_x * sum_x - request.points.size() * sum_xx;
            if (std::abs(b) > 1e-12)
            {
                response.slope = a / b;
                response.y_intercept = (sum_y - response.slope * sum_x) / request.points.size();
            }
        });
    if (res < 0)
    {
        throw std::runtime_error("Failed to start the PerformLinearLeastSquaresFit server: " + std::to_string(res));
    }

    /*
     * Running the node as usual.
     */
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

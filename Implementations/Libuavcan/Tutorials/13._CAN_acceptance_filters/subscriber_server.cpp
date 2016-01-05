#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/transport/can_acceptance_filter_configurator.hpp>

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
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    /*
     * Initializing a new node. Refer to the "Node initialization and startup" tutorial for more details.
     */
    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.subscriber");
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /**
     * The most common way to configure CAN hardware acceptance filters is just to use configureCanAcceptanceFilters()
     * method with a node you want to configure as an input argument. This method usually called after all the node's
     * subscribers have been declared and it calls computeConfiguration() and applyConfiguration() subsequently (these
     * functions will be described later in this tutorial).
     * At this point your HW filters will be configured to accept all service messages (always accepted by UAVCAN) and
     * anonymous messages, since we don't have any subscribers declared so far. You may already
     * execute publisher_client which will constantly send messages to this node.
     */
    configureCanAcceptanceFilters(node);

    /**
     * Initializing a subscriber uavcan::equipment::air_data::Sideslip. Please refer to the "Publishers and subscribers"
     * tutorial for more details.
     * We are still not getting any messages from publisher_client node, since the air_data::Sideslip was declared after
     * acceptance filters have been configured.
     */
    uavcan::Subscriber<uavcan::equipment::air_data::Sideslip> sideslip_sub(node);
    int sideslip_sub_start_res = sideslip_sub.start([&](const uavcan::equipment::air_data::Sideslip& msg)
    {
        std::cout << msg << std::endl;
    });
    if (sideslip_sub_start_res < 0)
    {
        throw std::runtime_error("Failed to start the key/value subscriber; error: " +
                                 std::to_string(sideslip_sub_start_res));
    }

    /**
     * Will not receive air_data::Sideslip messages for another 3 seconds until we reconfigure acceptance filters and
     * include another configuration for the message. As soon as filters are reconfigured the node will start to
     * receive the air_data::Sideslip messages.
     */
    node.spin(uavcan::MonotonicDuration::fromMSec(3000)); // Wait 3 seconds
    std::cout << std::endl << "Reconfiguring acceptance filters ..." << std::endl;
    configureCanAcceptanceFilters(node);

    /**
     * In case you need to create custom configuration of hardware acceptance filters, or if you want to output your
     * computed configurations, the following approach should be used.
     *
     * The first step to configure CAN hardware acceptance filters is to create an object of class
     * uavcan::CanAcceptanceFilterConfigurator with the desired node as an input argument.
     *
     * NOTICE: Only for this tutorial we are going to put the second argument, which overrides the actual number
     * of available hardware filters to 6. You are highly unlikely to ever need this feature - it's only provided to
     * make this tutorial more illustrative.
     * There is also MaxCanAcceptanceFilters parameter in libuavcan/include/uavcan/build_config.hpp that limits the
     * maximum number of CAN acceptance filters available on the platform. You may increase this number manually if
     * your platform provides more filters and you want to use all of them (there are very few CAN controllers that
     * feature more than 32 filters though, if any).
     *
     * This object haven't configured anything yet, in order to make the configuration proceed to the
     * next step.
     */
    uavcan::CanAcceptanceFilterConfigurator anon_test_configuration(node, 6/* not needed in real apps */);

    /*
     * Initializing another subscriber uavcan::equipment::air_data::TrueAirspeed.
     */
    uavcan::Subscriber<uavcan::equipment::air_data::TrueAirspeed> airspd_sub(node);
    int airspd_sub_sub_start_res = airspd_sub.start([&](const uavcan::equipment::air_data::TrueAirspeed& msg)
    {
        std::cout << msg << std::endl;
    });
    if (airspd_sub_sub_start_res < 0)
    {
        throw std::runtime_error("Failed to start the key/value subscriber; error: "
                                 + std::to_string(airspd_sub_sub_start_res));
    }

    /*
     * Initializing service server. Please refer to the "Services" tutorial for more details.
     */
    using uavcan::protocol::file::BeginFirmwareUpdate;
    uavcan::ServiceServer<BeginFirmwareUpdate> srv(node);
    int srv_start_res = srv.start(
        [&](const uavcan::ReceivedDataStructure<BeginFirmwareUpdate::Request>& req, BeginFirmwareUpdate::Response& rsp)
    {
        std::cout << req << std::endl;
        rsp.error = rsp.ERROR_UNKNOWN;
        rsp.optional_error_message = "I am filtered";
    });
    if (srv_start_res < 0)
    {
        throw std::runtime_error("Failed to start the server; error: " + std::to_string(srv_start_res));
    }

    /**
     * The method computeConfiguration() gathers information from all subscribers and service messages on the
     * configurator's node. Also it creates a container with automatically calculated configurations of Masks and
     * ID's and subsequently loads the configurations to the CAN controller.
     *
     * It may or may not take an argument:
     * - IgnoreAnonymousMessages
     * - AcceptAnonymousMessages (default, if no input arguments)
     *
     * By default filter configurator accepts all anonymous messages. If you don't want them, you may specify the
     * IgnoreAnonymousMessages input argument. Let's make configurator that accepts anonymous messages.
     */
    anon_test_configuration.computeConfiguration();

    /**
     * At this point you have your configuration calculated and stored within the class container multiset_configs_, but
     * it's not still applied. Let's take a look what is inside the container of configurations using method
     * getConfiguration(). The output should consist of four configurations: service, anonymous, air_data::TrueAirspeed
     * and air_data::Sideslip messages.
     */
    auto& configure_array = anon_test_configuration.getConfiguration();
    uint16_t configure_array_size = configure_array.getSize();

    std::cout << std::endl << "Configuration with AcceptAnonymousMessages input and two subscribers:" << std::endl;
    for (uint16_t i = 0; i < configure_array_size; i++)
    {
        std::cout << "config.ID [" << i << "]= " << configure_array.getByIndex(i)->id << std::endl;
        std::cout << "config.MK [" << i << "]= " << configure_array.getByIndex(i)->mask << std::endl;
    }
    std::cout << std::endl;

    /**
     * Our node keeps getting air_data::Sideslip messages since configureCanAcceptanceFilters() was called previously
     * and service message file::BeginFirmwareUpdate since the service was declared (always accepted by UAVCAN, again).
     * But the air_data::TrueAirspeed is still not being accepted, even though it's within the multiset_configs_
     * container. In order to carry out the configuration let's call applyConfiguration() in 3 seconds and start to
     * get another message.
     */
    node.spin(uavcan::MonotonicDuration::fromMSec(3000)); // Wait 3 seconds
    std::cout << std::endl << "Applying new configuration, air_data::TrueAirspeed is accepted now..." << std::endl;
    anon_test_configuration.applyConfiguration();

    node.spin(uavcan::MonotonicDuration::fromMSec(3000)); // Wait 3 seconds
    /**
     * If there is a need of adding new custom configuration to hardware filters you may utilize
     * addFilterConfig(CanFilterConfig& config) method. It must be called only after computeConfiguration() function is
     * called, otherwise you might lose your custom configurations. Every time computeConfiguration() is invoked, it
     * cleans the multiset_configs_ container.
     * Let's add 5 additional configurations:
     */
    uavcan::CanFilterConfig new_filter;
    for (uint16_t i = 1; i < 7; i++)
    {
        new_filter.mask = 255;
        new_filter.id = i * 2;
        anon_test_configuration.addFilterConfig(new_filter);
    }

    std::cout << std::endl << "Container after adding new custom configurations:" << std::endl;
    configure_array_size = configure_array.getSize();
    for (uint16_t i = 0; i < configure_array_size; i++)
    {
        std::cout << "config.ID [" << i << "]= " << configure_array.getByIndex(i)->id << std::endl;
        std::cout << "config.MK [" << i << "]= " << configure_array.getByIndex(i)->mask << std::endl;
    }

    /**
     * If the number of configurations within the container is higher than number of available filters (6 in this
     * tutorial), the excessive configurations will be merged in the most efficient way. The container has 10
     * configurations at the moment. Let's invoke applyConfiguration() method and display our container once again,
     * the number of configurations will be reduced to 6.
     */
    anon_test_configuration.applyConfiguration();

    std::cout << std::endl << "Container after adding new custom configurations and applyConfiguration():" << std::endl;
    configure_array_size = configure_array.getSize();
    for (uint16_t i = 0; i<configure_array_size; i++)
    {
        std::cout << "config.ID [" << i << "]= " << configure_array.getByIndex(i)->id << std::endl;
        std::cout << "config.MK [" << i << "]= " << configure_array.getByIndex(i)->mask << std::endl;
    }

    node.setModeOperational();

    while (true)
    {
        /*
         * The method spin() may return earlier if an error occurs (e.g. driver failure).
         * All error codes are listed in the header uavcan/error.hpp.
         */
        const int res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Implementations for the standard application-level functions are located in uavcan/protocol/.
 * The same path also contains the standard data types uavcan.protocol.*.
 */
#include <uavcan/protocol/global_time_sync_master.hpp>
#include <uavcan/protocol/global_time_sync_slave.hpp>


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

    const int self_node_id = std::stoi(argv[1]);

    uavcan::Node<NodeMemoryPoolSize> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.time_sync_master");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing the time sync master object.
     * No more than one time sync master can exist per node.
     */
    uavcan::GlobalTimeSyncMaster master(node);
    const int master_init_res = master.init();
    if (master_init_res < 0)
    {
        throw std::runtime_error("Failed to start the time sync master; error: " + std::to_string(master_init_res));
    }

    /*
     * A time sync master should be able to properly cooperate with redundant masters present in the
     * network - if there's a higher priority master, we need to switch to a slave mode and sync with that master.
     * Here we start a time sync slave for this purpose.
     * Remember that no more than one time sync slave can exist per node.
     */
    uavcan::GlobalTimeSyncSlave slave(node);
    const int slave_init_res = slave.start();
    if (slave_init_res < 0)
    {
        throw std::runtime_error("Failed to start the time sync slave; error: " + std::to_string(slave_init_res));
    }

    /*
     * Create a timer to publish the time sync message once a second.
     * Note that in real applications the logic governing time sync master can be even more complex,
     * i.e. if the local time source is not always available (like in GNSS receivers).
     */
    uavcan::Timer master_timer(node);
    master_timer.setCallback([&](const uavcan::TimerEvent&)
        {
            /*
             * Check whether there are higher priority masters in the network.
             * If there are, we need to activate the local slave in order to sync with them.
             */
            if (slave.isActive())  // "Active" means that the slave tracks at least one remote master in the network
            {
                if (node.getNodeID() < slave.getMasterNodeID())
                {
                    /*
                     * We're the highest priority master in the network.
                     * We need to suppress the slave now to prevent it from picking up unwanted sync messages from
                     * lower priority masters.
                     */
                    slave.suppress(true);  // SUPPRESS
                    std::cout << "I am the highest priority master; the next one has Node ID "
                              << int(slave.getMasterNodeID().get()) << std::endl;
                }
                else
                {
                    /*
                     * There is at least one higher priority master in the network.
                     * We need to allow the slave to adjust our local clock in order to be in sync.
                     */
                    slave.suppress(false);  // UNSUPPRESS
                    std::cout << "Syncing with a higher priority master with Node ID "
                              << int(slave.getMasterNodeID().get()) << std::endl;
                }
            }
            else
            {
                /*
                 * There are no other time sync masters in the network, so we're the only time source.
                 * The slave must be suppressed anyway to prevent it from disrupting the local clock if a new
                 * lower priority master suddenly appears in the network.
                 */
                slave.suppress(true);
                std::cout << "No other masters detected in the network" << std::endl;
            }
            /*
             * Publish the sync message now, even if we're not a higher priority master.
             * Other nodes will be able to pick the right master anyway.
             */
            const int res = master.publish();
            if (res < 0)
            {
                std::cout << "Time sync master transient failure: " << res << std::endl;
            }
        });

    master_timer.startPeriodic(uavcan::MonotonicDuration::fromMSec(1000));

    /*
     * Running the node.
     */
    node.setModeOperational();
    while (true)
    {
        const int spin_res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (spin_res < 0)
        {
            std::cerr << "Transient failure: " << spin_res << std::endl;
        }
    }
}

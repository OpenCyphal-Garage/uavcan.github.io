#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Implementations for the standard application-level functions are located in uavcan/protocol/.
 * The same path also contains the standard data types uavcan.protocol.*.
 */
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
    node.setName("org.uavcan.tutorial.time_sync_slave");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Starting the time sync slave.
     * No more than one time sync slave can exist per node.
     * Every time a slave is able to determine the clock phase error, it calls
     * the platform driver method uavcan::ISystemClock::adjustUtc(uavcan::UtcDuration adjustment).
     * Usually, a slave makes an adjustment every second or two, depending on the master's broadcast rate.
     */
    uavcan::GlobalTimeSyncSlave slave(node);
    const int slave_init_res = slave.start();
    if (slave_init_res < 0)
    {
        throw std::runtime_error("Failed to start the time sync slave; error: " + std::to_string(slave_init_res));
    }

    /*
     * Running the node.
     * The time sync slave works in the background and requires no attention at all.
     */
    node.setModeOperational();
    while (true)
    {
        const int spin_res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (spin_res < 0)
        {
            std::cerr << "Transient failure: " << spin_res << std::endl;
        }

        /*
         * Printing the slave status information once a second
         */
        const bool active = slave.isActive();                      // Whether it can sync with a remote master

        const int master_node_id = slave.getMasterNodeID().get();  // Returns an invalid Node ID if (active == false)

        const long msec_since_last_adjustment = (node.getMonotonicTime() - slave.getLastAdjustmentTime()).toMSec();

        std::printf("Time sync slave status:\n"
                    "    Active: %d\n"
                    "    Master Node ID: %d\n"
                    "    Last clock adjustment was %ld ms ago\n\n",
                    int(active), master_node_id, msec_since_last_adjustment);

        /*
         * Note that libuavcan employs two different time scales:
         *
         * Monotonic time - Represented by the classes uavcan::MonotonicTime and uavcan::MonotonicDuration.
         *                  This time is stable and monotonic; it measures the amount of time since some unspecified
         *                  moment in the past and cannot jump or significantly change rate.
         *                  On Linux it is accessed via clock_gettime(CLOCK_MONOTONIC, ...).
         *
         * UTC time - Represented by the classes uavcan::UtcTime and uavcan::UtcDuration.
         *            This is real time that can be (but is not necessarily) synchronized with the network time.
         *            This time is not stable in the sense that it can change rate and jump forward and backward
         *            in order to eliminate the difference with the global network time.
         *            Note that, despite its name, this time scale does not need to be strictly UTC, although it
         *            is recommended.
         *            On Linux it is accessed via gettimeofday(...).
         *
         * Both clocks are accessible via the methods INode::getMonotonicTime() and INode::getUtcTime().
         *
         * Note that the time representation is type safe as it is impossible to mix UTC and monotonic time in
         * the same expression (compilation will fail).
         */
        uavcan::MonotonicTime mono_time = node.getMonotonicTime();

        uavcan::UtcTime utc_time = node.getUtcTime();

        std::printf("Current time in seconds: Monotonic: %s   UTC: %s\n",
                    mono_time.toString().c_str(), utc_time.toString().c_str());

        mono_time += uavcan::MonotonicDuration::fromUSec(1234);

        utc_time += uavcan::UtcDuration::fromUSec(1234);

        std::printf("1234 usec later: Monotonic: %s   UTC: %s\n",
                    mono_time.toString().c_str(), utc_time.toString().c_str());
    }
}

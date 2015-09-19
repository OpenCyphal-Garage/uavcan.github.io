#include <iostream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

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

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.timers_cpp11");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Creating timers.
     * Timers are objects that instruct the libuavcan core to pass control to their callbacks either periodically
     * at specified interval, or once at some specific time point in the future.
     * Note that timer objects are noncopyable.
     *
     * A timer callback accepts a reference to an object of type uavcan::TimerEvent, which contains two fields:
     *  - The time when the callback was expected to be invoked.
     *  - The actual time when the callback was invoked.
     *
     * Timers do not require initialization and never fail (because of the very simple logic).
     *
     * Note that periodic timers do not accumulate phase error over time.
     */
    uavcan::Timer periodic_timer(node);

    uavcan::Timer one_shot_timer(node);

    periodic_timer.setCallback([&](const uavcan::TimerEvent& event)
        {
            node.logInfo("Periodic Timer", "scheduled_time: %*, real_time: %*",
                         event.scheduled_time.toMSec(), event.real_time.toMSec());

            // Timers can be checked whether there are pending events
            if (one_shot_timer.isRunning())
            {
                node.logError("Periodic Timer", "Someone started the one-shot timer! Period: %*",
                              one_shot_timer.getPeriod().toMSec());
                one_shot_timer.stop(); // And stopped like that
            }

            /*
             * Restart the second timer for one shot. It will be stopped automatically after the first event.
             * There are two ways to generate a one-shot timer event:
             *  - at a specified time point (absolute) - use the method startOneShotWithDeadline();
             *  - after a specified timeout (relative) - use the method startOneShotWithDelay().
             * Here we restart it in absolute mode.
             */
            auto one_shot_deadline = event.scheduled_time + uavcan::MonotonicDuration::fromMSec(200);
            one_shot_timer.startOneShotWithDeadline(one_shot_deadline);
        });

    one_shot_timer.setCallback([&](const uavcan::TimerEvent& event)
        {
            node.logInfo("One-Shot Timer", "scheduled_time: %*, real_time: %*",
                         event.scheduled_time.toMSec(), event.real_time.toMSec());
        });

    /*
     * Starting the timer at 1 Hz.
     * Start cannot fail.
     */
    periodic_timer.startPeriodic(uavcan::MonotonicDuration::fromMSec(1000));

    /*
     * Node loop.
     */
    node.getLogger().setLevel(uavcan::protocol::debug::LogLevel::DEBUG);
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

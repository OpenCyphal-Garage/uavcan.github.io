#include <iostream>
#include <sstream>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/util/method_binder.hpp>    // Sic!

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class demonstrates how to use uavcan::MethodBinder and uavcan::TimerEventForwarder in C++03.
 * In C++11 and newer standards it is recommended to use lambdas and std::function<> instead, as this approach
 * would be much easier to implement and to understand.
 */
class NodeWithTimers
{
    static const unsigned NodeMemoryPoolSize = 16384;

    uavcan::Node<NodeMemoryPoolSize> node_;

    // Instantiation of uavcan::MethodBinder
    typedef uavcan::MethodBinder<NodeWithTimers*, void (NodeWithTimers::*)(const uavcan::TimerEvent&)>
        TimerCallbackBinder;

    // Now we can use the newly instantiated timer type
    uavcan::TimerEventForwarder<TimerCallbackBinder> periodic_timer_;
    uavcan::TimerEventForwarder<TimerCallbackBinder> one_shot_timer_;

    void periodicCallback(const uavcan::TimerEvent& event)
    {
        std::cout << "Periodic: scheduled_time: " << event.scheduled_time
                  << ", real_time: " << event.real_time << std::endl;

        uavcan::MonotonicTime one_shot_deadline = event.scheduled_time + uavcan::MonotonicDuration::fromMSec(200);
        one_shot_timer_.startOneShotWithDeadline(one_shot_deadline);
    }

    void oneShotCallback(const uavcan::TimerEvent& event)
    {
        std::cout << "One-shot: scheduled_time: " << event.scheduled_time
                  << ", real_time: " << event.real_time << std::endl;
    }

public:
    NodeWithTimers() :
        node_(getCanDriver(), getSystemClock()),
        periodic_timer_(node_, TimerCallbackBinder(this, &NodeWithTimers::periodicCallback)),
        one_shot_timer_(node_, TimerCallbackBinder(this, &NodeWithTimers::oneShotCallback))
    {
        node_.getLogger().setLevel(uavcan::protocol::debug::LogLevel::DEBUG);
    }

    int start(uavcan::NodeID self_node_id, const std::string& node_name)
    {
        node_.setNodeID(self_node_id);
        node_.setName(node_name.c_str());
        return node_.start();
    }

    void runForever()
    {
        periodic_timer_.startPeriodic(uavcan::MonotonicDuration::fromMSec(1000));   // Start cannot fail

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

    NodeWithTimers node;

    const int node_start_res = node.start(std::atoi(argv[1]), "org.uavcan.tutorial.timers_cpp03");
    if (node_start_res < 0)
    {
        std::ostringstream os;
        os << "Failed to start the node; error: " << node_start_res;
        throw std::runtime_error(os.str());
    }

    node.runForever();
}

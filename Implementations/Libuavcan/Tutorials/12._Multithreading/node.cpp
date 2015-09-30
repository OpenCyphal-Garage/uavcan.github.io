/*
 * This demo shows how to implement a multi-threaded UAVCAN node with libuavcan.
 *
 * The main thread is publishing KeyValue messages, simulating a hard real-time task.
 *
 * The secondary thread is running an active node monitor (based on uavcan::NodeInfoRetriever),
 * which performs blocking filesystem I/O and therefore cannot be implemented in the main thread.
 */

/*
 * Standard C++ headers.
 */
#include <iostream>                     // For std::cout and std::cerr
#include <fstream>                      // For std::ofstream, which is used in the demo payload logic
#include <thread>                       // For std::thread

/*
 * Libuavcan headers.
 */
#include <uavcan/uavcan.hpp>            // Main libuavcan header
#include <uavcan/node/sub_node.hpp>     // For uavcan::SubNode, which is essential for multithreaded nodes

/*
 * These are purely for demonstrational purposes - they have no relation to multithreading.
 */
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/node_info_retriever.hpp>

/*
 * Demo implementation of a virtual interface.
 */
#include "uavcan_virtual_iface.hpp"

/*
 * These functions are explained in one of the first tutorials.
 */
extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class demonstrates a simple main node that is supposed to run some hard real-time tasks.
 */
class MainNodeDemo
{
    uavcan::Node<16384> node_;

public:
    MainNodeDemo(uavcan::NodeID self_node_id, const std::string& self_node_name) :
        node_(getCanDriver(), getSystemClock())
    {
        node_.setNodeID(self_node_id);
        node_.setName(self_node_name.c_str());
    }

    uavcan::INode& getNode() { return node_; }

    void runForever()
    {
        const int start_res = node_.start();
        if (start_res < 0)
        {
            throw std::runtime_error("Failed to start the main node: " + std::to_string(start_res));
        }

        /*
         * Initializing a KV publisher.
         * This publication simulates a hard-real time task for the main node.
         */
        uavcan::Publisher<uavcan::protocol::debug::KeyValue> kv_pub(node_);
        uavcan::Timer kv_pub_timer(node_);

        kv_pub_timer.startPeriodic(uavcan::MonotonicDuration::fromMSec(1000));

        kv_pub_timer.setCallback([&kv_pub](const uavcan::TimerEvent&)
            {
                uavcan::protocol::debug::KeyValue msg;
                msg.key = "Bob";
                msg.value = 1.0F / static_cast<float>(std::rand());
                (void)kv_pub.broadcast(msg);      // TODO: Error handling
            });

        /*
         * We know that in this implementation the class uavcan_virtual_iface::Driver inherits uavcan::IRxFrameListener,
         * so we can simply restore the reference to uavcan_virtual_iface::ITxQueueInjector using dynamic_cast<>.
         *
         * In other implementations this approach may be unacceptable (e.g. RTTI, which is required for dynamic_cast<>,
         * is often unavailable on deeply embedded systems), so a reference to uavcan_virtual_iface::ITxQueueInjector
         * will have to be passed here using some other means (e.g. as a reference to the constructor).
         */
        while (node_.getDispatcher().getRxFrameListener() == nullptr)
        {
            // Waiting for the sub-node to install the RX listener.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        auto& tx_injector =
            dynamic_cast<uavcan_virtual_iface::ITxQueueInjector&>(*node_.getDispatcher().getRxFrameListener());

        /*
         * Running the node ALMOST normally.
         *
         * Spinning must break every once in a while in order to unload TX queue of the sub-node into the
         * TX queue of the main node. Duration of spinning defines worst-case transmission delay for sub-node's
         * outgoing frames. Since sub-nodes are not supposed to run hard real-time processes, transmission delays
         * introduced by periodic queue synchronizations are acceptable.
         */
        node_.setModeOperational();

        while (true)
        {
            const int res = node_.spin(uavcan::MonotonicDuration::fromMSec(2));
            if (res < 0)
            {
                std::cerr << "Transient failure: " << res << std::endl;
            }
            // CAN frame transfer from sub-node to the main node occurs here.
            tx_injector.injectTxFramesInto(node_);
        }
    }
};

/**
 * This is just some demo logic, it has nothing to do with multithreading.
 * This class implements uavcan::INodeInfoListener, storing node info on the file system, one file per node.
 * Please refer to the tutorial "Node discovery" to learn more.
 */
class FileBasedNodeInfoCollector final : public uavcan::INodeInfoListener
{
    void handleNodeInfoRetrieved(uavcan::NodeID node_id,
                                 const uavcan::protocol::GetNodeInfo::Response& node_info) override
    {
        std::cout << "Node info for " << int(node_id.get()) << ":\n" << node_info << std::endl;
        std::ofstream(std::to_string(node_id.get())) << node_info;
    }

    void handleNodeInfoUnavailable(uavcan::NodeID node_id) override
    {
        std::cout << "Node info for " << int(node_id.get()) << " is unavailable" << std::endl;
        std::ofstream(std::to_string(node_id.get())) << uavcan::protocol::GetNodeInfo::Response();
    }

    void handleNodeStatusChange(const uavcan::NodeStatusMonitor::NodeStatusChangeEvent& event) override
    {
        if (event.status.mode == uavcan::protocol::NodeStatus::MODE_OFFLINE)
        {
            std::cout << "Node " << int(event.node_id.get()) << " went offline" << std::endl;
            (void)std::remove(std::to_string(event.node_id.get()).c_str());
        }
    }
};

/**
 * This class demonstrates a simple sub-node that is supposed to run CPU-intensive, blocking, non-realtime tasks.
 */
class SubNodeDemo
{
    static constexpr unsigned VirtualDriverQueuePoolSize = 20000;

    uavcan_virtual_iface::Driver<VirtualDriverQueuePoolSize> driver_;
    uavcan::SubNode<16384> node_;

    uavcan::NodeInfoRetriever retriever_;
    FileBasedNodeInfoCollector collector_;

public:
    /**
     * Sub-node needs a reference to the main node in order to bind its virtual CAN driver to it.
     */
    SubNodeDemo(uavcan::INode& main_node) :
        driver_(main_node.getDispatcher().getCanIOManager().getCanDriver().getNumIfaces(), // Nice?
                getSystemClock()),
        node_(driver_, getSystemClock()),
        retriever_(node_)
    {
        node_.setNodeID(main_node.getNodeID());                     // Obviously, we must use the same node ID.

        main_node.getDispatcher().installRxFrameListener(&driver_); // RX frames will be copied to the virtual driver.
    }

    void runForever()
    {
        /*
         * Initializing the demo payload.
         * Note that the payload doesn't know that it's being runned by a secondary node - on the application level,
         * there's no difference between a sub-node and the main node.
         */
        const int retriever_res = retriever_.start();
        if (retriever_res < 0)
        {
            throw std::runtime_error("Failed to start the retriever; error: " + std::to_string(retriever_res));
        }

        const int add_listener_res = retriever_.addListener(&collector_);
        if (add_listener_res < 0)
        {
            throw std::runtime_error("Failed to add listener; error: " + std::to_string(add_listener_res));
        }

        /*
         * Running the node normally.
         * Note that the SubNode class does not implement the start() method - there's nothing to start.
         */
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

    MainNodeDemo main_node(self_node_id, "org.uavcan.tutorial.multithreading");

    SubNodeDemo sub_node(main_node.getNode());

    std::thread secondary_thread(std::bind(&SubNodeDemo::runForever, &sub_node));

    main_node.runForever();

    if (secondary_thread.joinable())
    {
        secondary_thread.join();
    }
}

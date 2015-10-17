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
#include <uavcan/helpers/heap_based_pool_allocator.hpp> // In this example we're going to use heap-based allocator

/*
 * These are purely for demonstrational purposes - they have no relation to multithreading.
 */
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/node_info_retriever.hpp>

/*
 * Demo implementation of a virtual CAN driver.
 */
#include "uavcan_virtual_driver.hpp"

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
    /**
     * This value will be reported by the allocator (read below) via @ref getBlockCapacity().
     * The real limit (hard limit) can be configured separately, and its default value is twice the soft limit.
     */
    static constexpr unsigned AllocatorBlockCapacitySoftLimit = 250;

    /**
     * Libuavcan's allocators can be made thread-safe by means of providing optional template argument - a type
     * that will be instantiated within every thread-critical context.
     * Since this allocator will be shared between nodes working in different threads, it must be thread-safe.
     */
    struct AllocatorSynchronizer
    {
        std::mutex& getMutex()
        {
            static std::mutex m;
            return m;
        }
        AllocatorSynchronizer()  { getMutex().lock(); }
        ~AllocatorSynchronizer() { getMutex().unlock(); }
    };

    /**
     * It is also possible to use the default pool-based allocator here, but we'll use the heap-based one for
     * demonstrational purposes. The difference is that the heap-based allocator doesn't reserve the pool statically,
     * instead it takes the memory from the heap using std::malloc(), and then keeps it for future use. This allocator
     * has the following advantages over the default one:
     *
     *  - Lower memory footprint. This is because it only allocates memory when the node needs it. Once a block is
     *    allocated, it will be kept for future re-use, therefore it doesn't need to constantly access heap (otherwise
     *    that would be highly non-deterministic and cause heap fragmentation).
     *
     *  - Ability to shrink if the memory is no longer needed. For example, after the node has finished a certain
     *    operation that required a lot of memory, it can be reclaimed back by means of calling
     *    @ref uavcan::HeapBasedPoolAllocator::shrink(). This method will call std::free() for every block that is
     *    not currently in use by the node.
     *
     * Needless to say, initial allocations depend on std::malloc(), which may cause problems for real-time
     * applications, so this allocator should be used with care. If in doubt, use traditional one, since it also
     * can be made thread-safe. Or even use independent allocators per every (sub)node object, this is even more
     * deterministic, but takes much more memory.
     */
    uavcan::HeapBasedPoolAllocator<uavcan::MemPoolBlockSize, AllocatorSynchronizer> allocator_;

    /**
     * Note that we don't provide the pool size parameter to the @ref uavcan::Node template.
     * If no value is provided, it defaults to zero, making the node expect a reference to @ref uavcan::IPoolAllocator
     * to the constructor. This allows us to install a custom allocator, instantiated above.
     */
    uavcan::Node<> node_;

public:
    MainNodeDemo(uavcan::NodeID self_node_id, const std::string& self_node_name) :
        allocator_(AllocatorBlockCapacitySoftLimit),
        node_(getCanDriver(), getSystemClock(), allocator_)     // Installing our custom allocator.
    {
        node_.setNodeID(self_node_id);
        node_.setName(self_node_name.c_str());
    }

    uavcan::INode& getNode() { return node_; }

    /**
     * Needed only for demonstration.
     */
    unsigned getMemoryAllocatorFootprint() const
    {
        return allocator_.getNumReservedBlocks() * uavcan::MemPoolBlockSize;
    }

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
         * We know that in this implementation the class uavcan_virtual_driver::Driver inherits uavcan::IRxFrameListener,
         * so we can simply restore the reference to uavcan_virtual_driver::ITxQueueInjector using dynamic_cast<>.
         *
         * In other implementations this approach may be unacceptable (e.g. RTTI, which is required for dynamic_cast<>,
         * is often unavailable on deeply embedded systems), so a reference to uavcan_virtual_driver::ITxQueueInjector
         * will have to be passed here using some other means (e.g. as a reference to the constructor).
         */
        while (node_.getDispatcher().getRxFrameListener() == nullptr)
        {
            // Waiting for the sub-node to install the RX listener.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        auto& tx_injector =
            dynamic_cast<uavcan_virtual_driver::ITxQueueInjector&>(*node_.getDispatcher().getRxFrameListener());

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
    static constexpr unsigned BlockAllocationQuotaPerIface = 80;

    uavcan_virtual_driver::Driver driver_;
    uavcan::SubNode<> node_;

    uavcan::NodeInfoRetriever retriever_;
    FileBasedNodeInfoCollector collector_;

public:
    /**
     * Sub-node needs a reference to the main node in order to bind its virtual CAN driver to it.
     * Also, the sub-node and the virtual iface all use the same allocator from the main node (it is thread-safe).
     * It is also possible to use dedicated allocators for every entity, but that would lead to higher memory
     * footprint.
     */
    SubNodeDemo(uavcan::INode& main_node) :
        driver_(main_node.getDispatcher().getCanIOManager().getCanDriver().getNumIfaces(), // Nice?
                getSystemClock(),
                main_node.getAllocator(),       // Installing our custom allocator from the main node.
                BlockAllocationQuotaPerIface),
        node_(driver_,
              getSystemClock(),
              main_node.getAllocator()),        // Installing our custom allocator from the main node.
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

    // This thread is only needed for demo purposes; can be removed freely.
    std::thread allocator_stat_reporting_thread([&main_node]()
        {
            unsigned last_peak = 0;
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                const auto usage = main_node.getMemoryAllocatorFootprint();
                if (usage != last_peak)
                {
                    last_peak = usage;
                    std::cout << "Memory footprint: " << last_peak << " bytes" << std::endl;
                }
            }
        });

    main_node.runForever();

    if (secondary_thread.joinable())
    {
        secondary_thread.join();
    }
}

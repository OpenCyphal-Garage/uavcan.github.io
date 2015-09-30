#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/node_info_retriever.hpp>      // For uavcan::NodeInfoRetriever

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class will be collecting information from uavcan::NodeInfoRetriever via the interface uavcan::INodeInfoListener.
 * Please refer to the documentation for uavcan::NodeInfoRetriever to learn more.
 */
class NodeInfoCollector final : public uavcan::INodeInfoListener
{
    struct NodeIDHash
    {
        std::size_t operator()(uavcan::NodeID nid) const { return nid.get(); }
    };

    std::unordered_map<uavcan::NodeID, uavcan::protocol::GetNodeInfo::Response, NodeIDHash> registry_;

    /**
     * Called when a response to GetNodeInfo request is received. This happens shortly after the node restarts or
     * becomes online for the first time.
     * @param node_id   Node ID of the node
     * @param response  Node info struct
     */
    void handleNodeInfoRetrieved(uavcan::NodeID node_id,
                                 const uavcan::protocol::GetNodeInfo::Response& node_info) override
    {
        registry_[node_id] = node_info;
    }

    /**
     * Called when the retriever decides that the node does not support the GetNodeInfo service.
     * This method will never be called if the number of attempts is unlimited.
     */
    void handleNodeInfoUnavailable(uavcan::NodeID node_id) override
    {
        // In this implementation we're using empty struct to indicate that the node info is missing.
        registry_[node_id] = uavcan::protocol::GetNodeInfo::Response();
    }

    /**
     * This call is routed directly from @ref NodeStatusMonitor.
     * Default implementation does nothing.
     * @param event     Node status change event
     */
    void handleNodeStatusChange(const uavcan::NodeStatusMonitor::NodeStatusChangeEvent& event) override
    {
        if (event.status.mode == uavcan::protocol::NodeStatus::MODE_OFFLINE)
        {
            registry_.erase(event.node_id);
        }
    }

    /**
     * This call is routed directly from @ref NodeStatusMonitor.
     * Default implementation does nothing.
     * @param msg       Node status message
     */
    void handleNodeStatusMessage(const uavcan::ReceivedDataStructure<uavcan::protocol::NodeStatus>& msg) override
    {
        auto x = registry_.find(msg.getSrcNodeID());
        if (x != registry_.end())
        {
            x->second.status = msg;
        }
    }

public:
    /**
     * Number if known nodes in the registry.
     */
    std::uint8_t getNumberOfNodes() const
    {
        return static_cast<std::uint8_t>(registry_.size());
    }

    /**
     * Returns a pointer to the node info structure for the given node, if such node is known.
     * If the node is not known, a null pointer will be returned.
     * Note that the pointer may be invalidated afterwards, so the object MUST be copied if further use is intended.
     */
    const uavcan::protocol::GetNodeInfo::Response* getNodeInfo(uavcan::NodeID node_id) const
    {
        auto x = registry_.find(node_id);
        if (x != registry_.end())
        {
            return &x->second;
        }
        else
        {
            return nullptr;
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

    uavcan::Node<16384> node(getCanDriver(), getSystemClock());

    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.active_monitor");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing the node info retriever object.
     */
    uavcan::NodeInfoRetriever retriever(node);

    const int retriever_res = retriever.start();
    if (retriever_res < 0)
    {
        throw std::runtime_error("Failed to start the retriever; error: " + std::to_string(retriever_res));
    }

    /*
     * This class is defined above in this file.
     */
    NodeInfoCollector collector;

    /*
     * Registering our collector against the retriever object.
     * The retriever class may keep the pointers to listeners in the dynamic memory pool,
     * therefore the operation may fail if the node runs out of memory in the pool.
     */
    const int add_listener_res = retriever.addListener(&collector);
    if (add_listener_res < 0)
    {
        throw std::runtime_error("Failed to add listener; error: " + std::to_string(add_listener_res));
    }

    /*
     * Running the node.
     * The application will be updating the list of nodes in the console window.
     */
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(500));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }

        /*
         * Rendering the info to the console window.
         * Note that the window must be large in order to accommodate information for multiple nodes (use smaller font).
         */
        std::cout << "\x1b[1J"  // Clear screen from the current cursor position to the beginning
                  << "\x1b[H"   // Move cursor to the coordinates 1,1
                  << std::flush;

        for (std::uint8_t i = 1; i <= uavcan::NodeID::Max; i++)
        {
            if (auto p = collector.getNodeInfo(i))
            {
                std::cout << "\033[32m---------- " << int(i) << " ----------\033[39m\n" // This line will be colored
                          << *p << std::endl;
            }
        }
    }

    return 0;
}

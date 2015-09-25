#include <iostream>
#include <cstdlib>
#include <array>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/dynamic_node_id_server/distributed.hpp>

/*
 * We're using POSIX-dependent classes in this example.
 * This means that the example will only work as-is on a POSIX-compliant system (e.g. Linux, NuttX),
 * otherwise the said classes will have to be re-implemented.
 */
#include <uavcan_posix/dynamic_node_id_server/file_storage_backend.hpp>
#include <uavcan_posix/dynamic_node_id_server/file_event_tracer.hpp>

#if __linux__
/*
 * This inclusion is specific to Linux.
 * Refer to the function getUniqueID() below to learn why do we need this header.
 */
# include <uavcan_linux/system_utils.hpp>   // Pulls uavcan_linux::makeApplicationID()
#endif

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

static const std::string NodeName = "org.uavcan.tutorial.distributed_allocator";

/**
 * Refer to the Allocatee example to learn more about this function.
 */
static std::array<std::uint8_t, 16> getUniqueID(uint8_t instance_id)
{
#if __linux__
    return uavcan_linux::makeApplicationID(uavcan_linux::MachineIDReader().read(), NodeName, instance_id);
#else
# error "Add support for your platform"
#endif
}

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id> <cluster-size>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);
    const int cluster_size = std::stoi(argv[2]);

    /*
     * Configuring the node.
     */
    uavcan::Node<16384> node(getCanDriver(), getSystemClock());

    node.setNodeID(self_node_id);
    node.setName(NodeName.c_str());

    const auto unique_id = getUniqueID(self_node_id);                   // Using the node ID as instance ID

    uavcan::protocol::HardwareVersion hwver;
    std::copy(unique_id.begin(), unique_id.end(), hwver.unique_id.begin());
    std::cout << hwver << std::endl;                                    // Printing to stdout to show the values

    node.setHardwareVersion(hwver);                                     // Copying the value to the node's internals

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing the event tracer - refer to the Centralized Allocator example for details.
     */
    uavcan_posix::dynamic_node_id_server::FileEventTracer event_tracer;
    const int event_tracer_res = event_tracer.init("uavcan_db_distributed/event.log");  // Using a hard-coded path here.
    if (event_tracer_res < 0)
    {
        throw std::runtime_error("Failed to start the event tracer; error: " + std::to_string(event_tracer_res));
    }

    /*
     * Initializing the storage backend - refer to the Centralized Allocator example for details.
     */
    uavcan_posix::dynamic_node_id_server::FileStorageBackend storage_backend;
    const int storage_res = storage_backend.init("uavcan_db_distributed");              // Using a hard-coded path here.
    if (storage_res < 0)
    {
        throw std::runtime_error("Failed to start the storage backend; error: " + std::to_string(storage_res));
    }

    /*
     * Starting the allocator itself.
     */
    uavcan::dynamic_node_id_server::DistributedServer server(node, storage_backend, event_tracer);

    // USING THE SAME UNIQUE ID HERE
    const int server_init_res = server.init(node.getHardwareVersion().unique_id, cluster_size);
    if (server_init_res < 0)
    {
        throw std::runtime_error("Failed to start the server; error " + std::to_string(server_init_res));
    }

    std::cout << "Distributed server started successfully" << std::endl;

    /*
     * Running the node, and printing some basic status information of the server.
     */
    node.setModeOperational();
    while (true)
    {
        const int spin_res = node.spin(uavcan::MonotonicDuration::fromMSec(500));
        if (spin_res < 0)
        {
            std::cerr << "Transient failure: " << spin_res << std::endl;
        }

        /*
         * Printing some basic info.
         * The reader is adviced to refer to the dynamic node ID allocator application provided with the
         * Linux platform driver to see how to retrieve more detailed status information from the library.
         */
        std::cout << "\x1b[1J"  // Clear screen from the current cursor position to the beginning
                  << "\x1b[H"   // Move cursor to the coordinates 1,1
                  << std::flush;

        const auto time = node.getMonotonicTime();

        const auto raft_state_to_string = [](uavcan::dynamic_node_id_server::distributed::RaftCore::ServerState s)
        {
            switch (s)
            {
            case uavcan::dynamic_node_id_server::distributed::RaftCore::ServerStateFollower:  return "Follower";
            case uavcan::dynamic_node_id_server::distributed::RaftCore::ServerStateCandidate: return "Candidate";
            case uavcan::dynamic_node_id_server::distributed::RaftCore::ServerStateLeader:    return "Leader";
            default: return "BADSTATE";
            }
        };

        static const auto duration_to_string = [](uavcan::MonotonicDuration dur)
        {
            uavcan::MakeString<16>::Type str;   // N.B.: this is faster than std::string, as it doesn't use heap
            str.appendFormatted("%.1f", dur.toUSec() / 1e6);
            return str;
        };

        const uavcan::dynamic_node_id_server::distributed::StateReport report(server);

        std::cout << "Node ID           " << int(node.getNodeID().get()) << "\n"
                  << "State             " << raft_state_to_string(report.state) << "\n"
                  << "Last log index    " << int(report.last_log_index) << "\n"
                  << "Commit index      " << int(report.commit_index) << "\n"
                  << "Last log term     " << report.last_log_term << "\n"
                  << "Current term      " << report.current_term << "\n"
                  << "Voted for         " << int(report.voted_for.get()) << "\n"
                  << "Since activity    " << duration_to_string(time - report.last_activity_timestamp).c_str() << "\n"
                  << "Random timeout    " << duration_to_string(report.randomized_timeout).c_str() << "\n"
                  << "Unknown nodes     " << int(report.num_unknown_nodes) << "\n"
                  << "Node failures     " << node.getInternalFailureCount() << "\n"
                  << std::flush;
    }
}

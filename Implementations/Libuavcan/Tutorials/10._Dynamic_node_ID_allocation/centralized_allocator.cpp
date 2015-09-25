#include <iostream>
#include <cstdlib>
#include <array>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/dynamic_node_id_server/centralized.hpp>

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

static const std::string NodeName = "org.uavcan.tutorial.centralized_allocator";

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
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

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
     * Event tracer is used to log events from the allocator class.
     *
     * Each event contains two attributes: event code and event argument (64-bit signed integer).
     *
     * If such logging is undesirable, an empty tracer can be implemented through the interface
     * uavcan::dynamic_node_id_server::IEventTracer.
     *
     * The interface also provides a static method getEventName(), which maps event codes to human-readable names.
     *
     * The tracer used here just logs events to a text file.
     */
    uavcan_posix::dynamic_node_id_server::FileEventTracer event_tracer;
    const int event_tracer_res = event_tracer.init("uavcan_db_centralized/event.log");  // Using a hard-coded path here.
    if (event_tracer_res < 0)
    {
        throw std::runtime_error("Failed to start the event tracer; error: " + std::to_string(event_tracer_res));
    }

    /*
     * Storage backend implements the interface uavcan::dynamic_node_id_server::IStorageBackend.
     * It is used by the allocator to access and modify the persistent key/value storage, where it keeps data.
     *
     * The implementation used here uses the file system to keep the data, where file names are KEYS, and
     * the contents of the files are VALUES. Note that the allocator only uses ASCII alphanumeric keys and values.
     */
    uavcan_posix::dynamic_node_id_server::FileStorageBackend storage_backend;
    const int storage_res = storage_backend.init("uavcan_db_centralized");              // Using a hard-coded path here.
    if (storage_res < 0)
    {
        throw std::runtime_error("Failed to start the storage backend; error: " + std::to_string(storage_res));
    }

    /*
     * Starting the allocator itself.
     * Its constructor accepts references to the node, to the event tracer, and to the storage backend.
     */
    uavcan::dynamic_node_id_server::CentralizedServer server(node, storage_backend, event_tracer);

    // USING THE SAME UNIQUE ID HERE
    const int server_init_res = server.init(node.getHardwareVersion().unique_id);
    if (server_init_res < 0)
    {
        throw std::runtime_error("Failed to start the server; error " + std::to_string(server_init_res));
    }

    std::cout << "Centralized server started successfully" << std::endl;

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
         */
        std::cout << "\x1b[1J"  // Clear screen from the current cursor position to the beginning
                  << "\x1b[H"   // Move cursor to the coordinates 1,1
                  << std::flush;

        std::cout << "Node ID           " << int(node.getNodeID().get()) << "\n"
                  << "Node failures     " << node.getInternalFailureCount() << "\n"
                  << std::flush;
    }
}

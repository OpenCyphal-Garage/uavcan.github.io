#include <iostream>
#include <cstdlib>
#include <array>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/dynamic_node_id_client.hpp>

#if __linux__
/*
 * This inclusion is specific to Linux.
 * Refer to the function getUniqueID() below to learn why do we need this header.
 */
# include <uavcan_linux/system_utils.hpp>   // Pulls uavcan_linux::makeApplicationID()
#endif

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

static const std::string NodeName = "org.uavcan.tutorial.allocatee";

/**
 * This function is supposed to obtain the unique ID from the hardware the node is running on.
 * The source of the unique ID depends on the platform; the examples provided below are valid for some of the
 * popular platforms.
 *
 * - STM32:
 *      The unique ID can be read from the Unique Device ID Register.
 *      Its location varies from family to family; please refer to the user manual to find out the correct address
 *      for your STM32. Note that the ID is 96-bit long, so it will have to be extended to 128-bit with zeros,
 *      or with some additional vendor-specific values.
 *
 * - LPC11C24:
 *      The 128-bit unique ID can be read using the IAP command ReadUID.
 *      For example:
 *      void readUniqueID(std::uint8_t out_uid[UniqueIDSize])
 *      {
 *          unsigned aligned_array[5] = {};  // out_uid may be unaligned, so we need to use temp array
 *          unsigned iap_command = 58;
 *          reinterpret_cast<void(*)(void*, void*)>(0x1FFF1FF1)(&iap_command, aligned_array);
 *          std::memcpy(out_uid, &aligned_array[1], 16);
 *      }
 *
 * - Linux:
 *      Vast majority of Linux distributions (possibly all?) provide means of obtaining the unique ID from the hardware.
 *      Libuavcan provides a class named uavcan_linux::MachineIDReader and a function uavcan_linux::makeApplicationID(),
 *      which allow to automatically determine the unique ID of the hardware the application is running on.
 *      Please refer to their definitions to learn more.
 *      In this example, we're using uavcan_linux::makeApplicationID().
 */
static std::array<std::uint8_t, 16> getUniqueID()
{
#if __linux__
    /*
     * uavcan_linux::MachineIDReader merely reads the ID of the hardware, whereas the function
     * uavcan_linux::makeApplicationID() is more complicated - it takes the following two or three inputs,
     * and then hashes them together to produce a unique ID:
     *  - Machine ID, as obtained from uavcan_linux::MachineIDReader.
     *  - Name of the node, as a string.
     *  - Instance ID (OPTIONAL), which allows to distinguish different instances of similarly named nodes
     *    running on the same Linux machine.
     *
     * The reason we can't use the machine ID directly is that it wouldn't allow us to run more than one UAVCAN node
     * on a given Linux system, as it would lead to multiple nodes having the same unique ID, which is prohibited.
     * The function makeApplicationID() works around that problem by means of extending the unique ID with a hash of
     * the name of the local node, which allows to run differently named nodes on the same Linux system without
     * conflicts.
     *
     * In situations where it is necessary to run multiple identically named nodes on the same Linux system, the
     * function makeApplicationID() can be supplied with the third argument, representing the ID of the instance.
     * The instance ID does only have to be unique for the given node name on the given Linux system, so
     * avoiding conflicts should be pretty straightforward.
     *
     * In this example we don't want to run the application in multiple instances,
     * therefore the instance ID is not used.
     */
    return uavcan_linux::makeApplicationID(uavcan_linux::MachineIDReader().read(), NodeName);
#else
# error "Add support for your platform"
#endif
}

int main(int argc, const char** argv)
{
    /*
     * The dynamic node ID allocation protocol allows the allocatee to ask the allocator for some particular node ID
     * value, if necessary. This feature is optional. By default, if no preference has been declared, the allocator
     * will pick any free node ID at its own discretion.
     */
    int preferred_node_id = 0;
    if (argc > 1)
    {
        preferred_node_id = std::stoi(argv[1]);
    }
    else
    {
        std::cout << "No preference for a node ID value.\n"
                  << "To assign a preferred node ID, pass it as a command line argument:\n"
                  << "\t" << argv[0] << " <preferred-node-id>" << std::endl;
    }

    /*
     * Configuring the node.
     */
    uavcan::Node<16384> node(getCanDriver(), getSystemClock());

    node.setName(NodeName.c_str());

    const auto unique_id = getUniqueID();                               // Reading the unique ID of this node

    uavcan::protocol::HardwareVersion hwver;
    std::copy(unique_id.begin(), unique_id.end(), hwver.unique_id.begin());
    std::cout << hwver << std::endl;                                    // Printing to stdout to show the values

    node.setHardwareVersion(hwver);                                     // Copying the value to the node's internals

    /*
     * Starting the node normally, in passive mode (i.e. without node ID assigned).
     */
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing the dynamic node ID allocation client.
     * By default, the client will use TransferPriority::OneHigherThanLowest for communications with the allocator;
     * this can be overriden through the third argument to the start() method.
     */
    uavcan::DynamicNodeIDClient client(node);

    int client_start_res = client.start(node.getHardwareVersion().unique_id,    // USING THE SAME UNIQUE ID AS ABOVE
                                        preferred_node_id);
    if (client_start_res < 0)
    {
        throw std::runtime_error("Failed to start the dynamic node ID client: " + std::to_string(client_start_res));
    }

    /*
     * Waiting for the client to obtain for us a node ID.
     * This may take a few seconds.
     */
    std::cout << "Allocation is in progress" << std::flush;
    while (!client.isAllocationComplete())
    {
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(200));    // Spin duration doesn't matter
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
        std::cout << "." << std::flush;
    }
    std::cout << "\nDynamic node ID "
              << int(client.getAllocatedNodeID().get())
              << " has been allocated by the allocator with node ID "
              << int(client.getAllocatorNodeID().get()) << std::endl;

    /*
     * When the allocation is done, the client object can be deleted.
     * Now we need to assign the newly allocated ID to the node object.
     */
    node.setNodeID(client.getAllocatedNodeID());

    /*
     * Now we can run the node normally.
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

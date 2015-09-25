#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Classes that implement high-level protocol logic
 */
#include <uavcan/protocol/firmware_update_trigger.hpp>  // uavcan::FirmwareUpdateTrigger
#include <uavcan/protocol/node_info_retriever.hpp>      // uavcan::NodeInfoRetriever (see tutorial "Node discovery")

/*
 * We're using POSIX-dependent classes in this example.
 * This means that the example will only work as-is on a POSIX-compliant system (e.g. Linux, NuttX),
 * otherwise the said classes will have to be re-implemented.
 */
#include <uavcan_posix/basic_file_server_backend.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class implements the application-specific part of uavcan::FirmwareUpdateTrigger
 * via the interface uavcan::IFirmwareVersionChecker.
 *
 * It works as follows: uavcan::FirmwareUpdateTrigger is subscribed to node information reports from
 * uavcan::NodeInfoRetriever via the interface uavcan::INodeInfoListener (learn more on node monitoring
 * in the tutorial "Node discovery"). Whenever FirmwareUpdateTrigger receives information about a new node,
 * it relays this information to the application via the interface uavcan::IFirmwareVersionChecker.
 * The application is then expected to check, using the information provided, whether the node requires
 * a firmware update, and reports the results of the check back to FirmwareUpdateTrigger. If the node requires
 * an update, FirmwareUpdateTrigger will send it a request uavcan.protocol.file.BeginFirmwareUpdate; otherwise
 * the node will be ignored until it restarts or re-appears on the bus. If the node did not respond to the update
 * request, FirmwareUpdateTrigger will try again. If the node responded with an error, FirmwareUpdateTrigger
 * will ask the application, using the same interface, whether it needs to try again.
 *
 * Refer to the source of uavcan::FirmwareUpdateTrigger to read more documentation.
 */
class ExampleFirmwareVersionChecker final : public uavcan::IFirmwareVersionChecker
{
    /**
     * This method will be invoked when the class obtains a response to GetNodeInfo request.
     *
     * @param node_id                   Node ID that this GetNodeInfo response was received from.
     *
     * @param node_info                 Actual node info structure; refer to uavcan.protocol.GetNodeInfo for details.
     *
     * @param out_firmware_file_path    The implementation should return the firmware image path via this argument.
     *                                  Note that this path must be reachable via uavcan.protocol.file.Read service.
     *                                  Refer to @ref FileServer and @ref BasicFileServer for details.
     *
     * @return                          True - the class will begin sending update requests.
     *                                  False - the node will be ignored, no request will be sent.
     */
    bool shouldRequestFirmwareUpdate(uavcan::NodeID node_id,
                                     const uavcan::protocol::GetNodeInfo::Response& node_info,
                                     FirmwareFilePath& out_firmware_file_path)
    override
    {
        /*
         * We need to make the decision, given the inputs, whether the node requires an update.
         * This part of the logic is deeply application-specific, so the solution provided here may not work
         * for real-world applications.
         */
        (void)node_id;
        (void)node_info;
        (void)out_firmware_file_path;
        return false;   // TODO
    }

    /**
     * This method will be invoked when a node responds to the update request with an error. If the request simply
     * times out, this method will not be invoked.
     * Note that if by the time of arrival of the response the node is already removed, this method will not be called.
     *
     * SPECIAL CASE: If the node responds with ERROR_IN_PROGRESS, the class will assume that further requesting
     *               is not needed anymore. This method will not be invoked.
     *
     * @param node_id                   Node ID that returned this error.
     *
     * @param error_response            Contents of the error response. It contains error code and text.
     *
     * @param out_firmware_file_path    New firmware path if a retry is needed. Note that this argument will be
     *                                  initialized with old path, so if the same path needs to be reused, this
     *                                  argument should be left unchanged.
     *
     * @return                          True - the class will continue sending update requests with new firmware path.
     *                                  False - the node will be forgotten, new requests will not be sent.
     */
    bool shouldRetryFirmwareUpdate(uavcan::NodeID node_id,
                                   const uavcan::protocol::file::BeginFirmwareUpdate::Response& error_response,
                                   FirmwareFilePath& out_firmware_file_path)
    override
    {
        /*
         * In this implementation we cancel the update request if the node responds with an error.
         */
        std::cout << "The node " << int(node_id.get()) << " has rejected the update request; file path was:\n"
                  << "\t" << out_firmware_file_path.c_str()
                  << "\nresponse was:\n"
                  << error_response << std::endl;
        return false;
    }

    /**
     * This node is invoked when the node responds to the update request with confirmation.
     * Note that if by the time of arrival of the response the node is already removed, this method will not be called.
     *
     * Implementation is optional; default one does nothing.
     *
     * @param node_id   Node ID that confirmed the request.
     *
     * @param response  Actual response.
     */
    void handleFirmwareUpdateConfirmation(uavcan::NodeID node_id,
                                          const uavcan::protocol::file::BeginFirmwareUpdate::Response& response)
    override
    {
        std::cout << "Node " << int(node_id.get()) << " has confirmed the update request; response was:\n"
                  << response << std::endl;
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

    /*
     * Initializing the node.
     *
     * Typically, a node that serves as a firmware updater will also implement a dynamic node ID allocator.
     * Refer to the tutorial "Dynamic node ID allocation" to learn how to add a dynamic node ID allocator
     * to your node using libuavcan.
     */
    uavcan::Node<16384> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.updater");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Initializing the node info retriever.
     *
     * We don't need it, but it will be used by the firmware version checker, which will be initialized next.
     */
    uavcan::NodeInfoRetriever node_info_retriever(node);

    const int retriever_res = node_info_retriever.start();
    if (retriever_res < 0)
    {
        throw std::runtime_error("Failed to start the node info retriever: " + std::to_string(retriever_res));
    }

    /*
     * Initializing the firmware update trigger.
     *
     * This class monitors the output of uavcan::NodeInfoRetriever, and using this output decides which nodes need
     * to update their firmware. When a node that requires an update is detected, the class sends a service request
     * uavcan.protocol.file.BeginFirmwareUpdate to it.
     *
     * The application-specific logic that performs the checks is implemented in the class
     * ExampleFirmwareVersionChecker, defined above in this file.
     */
    ExampleFirmwareVersionChecker checker;

    uavcan::FirmwareUpdateTrigger trigger(node, checker);

    const int trigger_res = trigger.start(node_info_retriever);
    if (trigger_res < 0)
    {
        throw std::runtime_error("Failed to start the firmware update trigger: " + std::to_string(trigger_res));
    }

    /*
     * Initializing the file server.
     *
     * It is not necessary to run the file server on the same node with the firmware update trigger
     * (this is explained in the specification), but this use case is the most common, so we'll demonstrate it here.
     */
    uavcan_posix::BasicFileServerBackend file_server_backend(node);
    uavcan::FileServer file_server(node, file_server_backend);

    const int file_server_res = file_server.start();
    if (file_server_res < 0)
    {
        throw std::runtime_error("Failed to start the file server: " + std::to_string(file_server_res));
    }

    /*
     * Running the node normally.
     * All of the work will be done in background.
     */
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

#include <iostream>
#include <cstdlib>
#include <cctype>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Classes that implement high-level protocol logic
 */
#include <uavcan/protocol/firmware_update_trigger.hpp>  // uavcan::FirmwareUpdateTrigger
#include <uavcan/protocol/node_info_retriever.hpp>      // uavcan::NodeInfoRetriever (see tutorial "Node discovery")

/*
 * We're using POSIX-dependent classes and POSIX API in this example.
 * This means that the example will only work as-is on a POSIX-compliant system (e.g. Linux, NuttX),
 * otherwise the said classes will have to be re-implemented.
 */
#include <uavcan_posix/basic_file_server_backend.hpp>
#include <glob.h>                                       // POSIX glob() function

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
         * for some real-world applications.
         *
         * It is recommended to refer to the PX4 autopilot project or to the APM project for a real-world
         * example.
         *
         * Both PX4 and APM leverage a class provided by the libuavcan's POSIX platform driver -
         * uavcan_posix::FirmwareVersionChecker - which implements a generic firmware version checking algorithm.
         * The algorithm works as follows:
         *   1. Check if the file system has a firmware file for the given node.
         *      If not, exit - update will not be possible anyway.
         *   2. Compare the CRC of the local firmware image for the given node with CRC of the firmware the node is
         *      running at the moment (the latter is available via the node info argument in this method).
         *   3. Request an update if CRC don't match, otherwise do not request an update.
         *
         * In this example, we're using a simpler logic.
         *
         * Firmware file name pattern used in the example is as follows:
         *      <node-name>-<hw-major>.<hw-minor>-<sw-major>.<sw-minor>.<vcs-hash-hex>.uavcan.bin
         */

        std::cout << "Checking firmware version of node " << int(node_id.get()) << "; node info:\n"
                  << node_info << std::endl;

        /*
         * Looking for matching firmware files.
         */
        auto files = findAvailableFirmwareFiles(node_info);
        if (files.empty())
        {
            std::cout << "No firmware files found for this node" << std::endl;
            return false;
        }

        std::cout << "Matching firmware files:";
        for (auto x: files)
        {
            std::cout << "\n\t" << x << "\n" << parseFirmwareFileName(x.c_str()) << std::endl;
        }

        /*
         * Looking for the firmware file with highest version number
         */
        std::string best_file_name;
        unsigned best_combined_version = 0;
        for (auto x: files)
        {
            const auto inf = parseFirmwareFileName(x.c_str());
            const unsigned combined_version = (unsigned(inf.software_version.major) << 8) + inf.software_version.minor;
            if (combined_version >= best_combined_version)
            {
                best_combined_version = combined_version;
                best_file_name = x;
            }
        }

        std::cout << "Preferred firmware: " << best_file_name << std::endl;

        const auto best_firmware_info = parseFirmwareFileName(best_file_name.c_str());

        /*
         * Comparing the best firmware with the actual one, requesting an update if they differ.
         */
        if (best_firmware_info.software_version.major == node_info.software_version.major &&
            best_firmware_info.software_version.minor == node_info.software_version.minor &&
            best_firmware_info.software_version.vcs_commit == node_info.software_version.vcs_commit)
        {
            std::cout << "Firmware is already up-to-date" << std::endl;
            return false;
        }

        /*
         * The current implementation of FirmwareUpdateTrigger imposes a limitation on the maximum length of
         * the firmware file path: it must not exceed 40 characters. This is NOT a limitation of UAVCAN itself.
         *
         * The firmware file name format we're currently using may procude names that exceed the length limitation,
         * therefore we need to work around that. We do so by means of computing a hash (CRC64 in this example, but
         * it obviously could be any other hash), and using it as the name of symlink to the firmware file.
         *
         * Aside from complying with the library's limitation, use of shorter lengths allows to somewhat reduce bus
         * traffic resulting from file read requests from the updatee, as every request carries the name of the file.
         *
         * TODO: Do not use symlink if file name length does not exceed the limit.
         */
        out_firmware_file_path = makeFirmwareFileSymlinkName(best_file_name.c_str(), best_file_name.length());

        (void)std::remove(out_firmware_file_path.c_str());

        const int symlink_res = ::symlink(best_file_name.c_str(), out_firmware_file_path.c_str());
        if (symlink_res < 0)
        {
            std::cout << "Could not create symlink: " << symlink_res << std::endl;
            return false;
        }

        std::cout << "Firmware file symlink: " << out_firmware_file_path.c_str() << std::endl;
        return true;
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

    /*
     * This function is specific for this example implementation.
     * It computes the name of a symlink to the firmware file.
     * Please see explanation above, where the function is called from.
     * The implementation is made so that it can work even on a deeply embedded system.
     */
    static FirmwareFilePath makeFirmwareFileSymlinkName(const char* file_name, unsigned file_name_length)
    {
        uavcan::DataTypeSignatureCRC hash;
        hash.add(reinterpret_cast<const std::uint8_t*>(file_name), file_name_length);
        auto hash_val = hash.get();

        static const char Charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        static const unsigned CharsetSize = sizeof(Charset) - 1;

        FirmwareFilePath out;
        while (hash_val > 0)
        {
            out.push_back(Charset[hash_val % CharsetSize]);
            hash_val /= CharsetSize;
        }

        out += ".bin";
        return out;
    }

    /*
     * This function is specific for this example implementation.
     * It extracts the version information from firmware file name.
     * The implementation is made so that it can work even on a deeply embedded system.
     * Assumed format is:
     *      <node-name>-<hw-major>.<hw-minor>-<sw-major>.<sw-minor>.<vcs-hash-hex>.uavcan.bin
     */
    static uavcan::protocol::GetNodeInfo::Response parseFirmwareFileName(const char* name)
    {
        // Must be static in order to avoid heap allocation
        static const auto extract_uint8 = [](unsigned& pos, const char* name)
        {
            std::uint8_t res = 0;
            pos++;
            while (std::isdigit(name[pos]))
            {
                res = res * 10 + int(name[pos] - '0');
                pos++;
            }
            return res;
        };

        uavcan::protocol::GetNodeInfo::Response res;
        unsigned pos = 0;

        while (name[pos] != '-')
        {
            res.name.push_back(name[pos]);
            pos++;
        }

        res.hardware_version.major = extract_uint8(pos, name);
        res.hardware_version.minor = extract_uint8(pos, name);
        res.software_version.major = extract_uint8(pos, name);
        res.software_version.minor = extract_uint8(pos, name);

        pos++;
        res.software_version.vcs_commit = ::strtoul(name + pos, nullptr, 16);
        res.software_version.optional_field_flags = res.software_version.OPTIONAL_FIELD_FLAG_VCS_COMMIT;

        return res;
    }

    /*
     * This function is specific for this example implementation.
     * It returns the firmware files available for given node info struct.
     */
    static std::vector<std::string> findAvailableFirmwareFiles(const uavcan::protocol::GetNodeInfo::Response& info)
    {
        std::vector<std::string> glob_matches;

        const std::string glob_pattern = std::string(info.name.c_str()) + "-" +
                                         std::to_string(info.hardware_version.major) + "." +
                                         std::to_string(info.hardware_version.minor) + "-*.uavcan.bin";

        auto result = ::glob_t();

        const int res = ::glob(glob_pattern.c_str(), 0, nullptr, &result);
        if (res != 0)
        {
            ::globfree(&result);
            if (res == GLOB_NOMATCH)
            {
                return glob_matches;
            }
            throw std::runtime_error("Can't glob()");
        }

        for(unsigned i = 0; i < result.gl_pathc; ++i)
        {
            glob_matches.emplace_back(result.gl_pathv[i]);
        }

        ::globfree(&result);

        return glob_matches;
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
     *
     * Also keep in mind that in most real-world applications, features dependent on blocking APIs
     * (such as this firmware update feature) will have to be implemented in a secondary thread in order to not
     * interfere with real-time processing of the primary thread. In the case of this firmware updater, the
     * interference may be caused by relatively intensive processing and blocking calls to the file system API.
     * Please refer to the dedicated tutorial to learn how to implement a multi-threaded node, then use that
     * knowledge to make this example application multithreaded (consider this an excercise).
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

    std::cout << "Started successfully" << std::endl;

    /*
     * Running the node normally.
     * All of the work will be done in background.
     */
    node.setModeOperational();
    while (true)
    {
        const int res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

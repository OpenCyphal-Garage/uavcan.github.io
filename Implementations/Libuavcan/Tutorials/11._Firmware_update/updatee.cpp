#include <iostream>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Data types used.
 */
#include <uavcan/protocol/file/BeginFirmwareUpdate.hpp>
#include <uavcan/protocol/file/Read.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

/**
 * This class downloads a firmware image from specified location to memory.
 * Download will start immediately after the object is constructed,
 * and it can be cancelled by means of deleting the object.
 *
 * This is just a made-up example - real applications will likely behave differently, either:
 * - Downloading the image using a dedicated bootloader application.
 * - Downloading the image to a file, that will be deployed later.
 * - Possibly something else, depending on the requirements of the application.
 */
class FirmwareLoader : private uavcan::TimerBase
{
public:
    /**
     * State transitions:
     *  InProgress ---[after background work]----> Success
     *                                       \ --> Failure
     */
    enum class Status
    {
        InProgress,
        Success,
        Failure
    };

private:
    const uavcan::NodeID source_node_id_;
    const uavcan::protocol::file::Path::FieldTypes::path source_path_;

    std::vector<std::uint8_t> image_;

    typedef uavcan::MethodBinder<FirmwareLoader*,
        void (FirmwareLoader::*)(const uavcan::ServiceCallResult<uavcan::protocol::file::Read>&)>
            ReadResponseCallback;

    uavcan::ServiceClient<uavcan::protocol::file::Read, ReadResponseCallback> read_client_;

    Status status_ = Status::InProgress;

    void handleTimerEvent(const uavcan::TimerEvent&) final override
    {
        if (!read_client_.hasPendingCalls())
        {
            uavcan::protocol::file::Read::Request req;
            req.path.path = source_path_;
            req.offset = image_.size();

            const int res = read_client_.call(source_node_id_, req);
            if (res < 0)
            {
                std::cerr << "Read call failed: " << res << std::endl;
            }
        }
    }

    void handleReadResponse(const uavcan::ServiceCallResult<uavcan::protocol::file::Read>& result)
    {
        if (result.isSuccessful() &&
            result.getResponse().error.value == 0)
        {
            auto& data = result.getResponse().data;

            image_.insert(image_.end(), data.begin(), data.end());

            if (data.size() < data.capacity())  // Termination condition
            {
                status_ = Status::Success;
                uavcan::TimerBase::stop();
            }
        }
        else
        {
            status_ = Status::Failure;
            uavcan::TimerBase::stop();
        }
    }

public:
    /**
     * Download will start immediately upon construction.
     * Destroy the object to cancel the process.
     */
    FirmwareLoader(uavcan::INode& node,
                   uavcan::NodeID source_node_id,
                   const uavcan::protocol::file::Path::FieldTypes::path& source_path) :
        uavcan::TimerBase(node),
        source_node_id_(source_node_id),
        source_path_(source_path),
        read_client_(node)
    {
        image_.reserve(1024);   // Arbitrary value

        /*
         * According to the specification, response priority equals request priority.
         * Typically, file I/O should be executed at a very low priority level.
         */
        read_client_.setPriority(uavcan::TransferPriority::OneHigherThanLowest);
        read_client_.setCallback(ReadResponseCallback(this, &FirmwareLoader::handleReadResponse));

        /*
         * Rate-limiting is necessary to avoid bus congestion.
         * The exact rate depends on the application's requirements and CAN bit rate.
         */
        uavcan::TimerBase::startPeriodic(uavcan::MonotonicDuration::fromMSec(200));
    }

    /**
     * This method allows to detect when the downloading is done, and whether it was successful.
     */
    Status getStatus() const { return status_; }

    /**
     * Returns the downloaded image.
     */
    const std::vector<std::uint8_t>& getImage() const { return image_; }
};

/**
 * This function is used to display the downloaded image.
 */
template <typename InputIterator>
void printHexDump(InputIterator begin, const InputIterator end)
{
    struct RAIIFlagsSaver
    {
        const std::ios::fmtflags flags_ = std::cout.flags();
        ~RAIIFlagsSaver() { std::cout.flags(flags_); }
    } _flags_saver;

    static constexpr unsigned BytesPerRow = 16;
    unsigned offset = 0;

    std::cout << std::hex << std::setfill('0');

    do
    {
        std::cout << std::setw(8) << offset << "  ";
        offset += BytesPerRow;

        {
            auto it = begin;
            for (unsigned i = 0; i < BytesPerRow; ++i)
            {
                if (i == 8)
                {
                    std::cout << ' ';
                }

                if (it != end)
                {
                    std::cout << std::setw(2) << unsigned(*it) << ' ';
                    ++it;
                }
                else
                {
                    std::cout << "   ";
                }
            }
        }

        std::cout << "  ";
        for (unsigned i = 0; i < BytesPerRow; ++i)
        {
            if (begin != end)
            {
                std::cout << ((unsigned(*begin) >= 32U && unsigned(*begin) <= 126U) ? char(*begin) : '.');
                ++begin;
            }
            else
            {
                std::cout << ' ';
            }
        }

        std::cout << std::endl;
    }
    while (begin != end);
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
     * Initializing the node.
     * Hardware and software version information is paramount for firmware update process.
     */
    uavcan::Node<16384> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.updatee");

    uavcan::protocol::HardwareVersion hwver;    // TODO initialize correct values
    hwver.major = 1;
    node.setHardwareVersion(hwver);

    uavcan::protocol::SoftwareVersion swver;    // TODO initialize correct values
    swver.major = 1;
    node.setSoftwareVersion(swver);

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Storage for the firmware downloader object.
     * Can be replaced with a smart pointer instead.
     */
    uavcan::LazyConstructor<FirmwareLoader> fw_loader;

    /*
     * Initializing the BeginFirmwareUpdate server.
     */
    uavcan::ServiceServer<uavcan::protocol::file::BeginFirmwareUpdate> bfu_server(node);

    const int bfu_res = bfu_server.start(
        [&fw_loader, &node]
        (const uavcan::ReceivedDataStructure<uavcan::protocol::file::BeginFirmwareUpdate::Request>& req,
         uavcan::protocol::file::BeginFirmwareUpdate::Response resp)
        {
            std::cout << "Firmware update request:\n" << req << std::endl;

            if (fw_loader.isConstructed())
            {
                resp.error = resp.ERROR_IN_PROGRESS;
            }
            else
            {
                const auto source_node_id = (req.source_node_id == 0) ? req.getSrcNodeID() : req.source_node_id;

                fw_loader.construct<uavcan::INode&, uavcan::NodeID, decltype(req.image_file_remote_path.path)>
                    (node, source_node_id, req.image_file_remote_path.path);
            }

            std::cout << "Response:\n" << resp << std::endl;
        });

    if (bfu_res < 0)
    {
        throw std::runtime_error("Failed to start the BeginFirmwareUpdate server: " + std::to_string(bfu_res));
    }

    /*
     * Running the node normally.
     * All of the work will be done in background.
     */
    while (true)
    {
        if (fw_loader.isConstructed())
        {
            node.setModeSoftwareUpdate();

            if (fw_loader->getStatus() != FirmwareLoader::Status::InProgress)
            {
                if (fw_loader->getStatus() == FirmwareLoader::Status::Success)
                {
                    auto image = fw_loader->getImage();

                    std::cout << "Firmware download succeeded [" << image.size() << " bytes]" << std::endl;
                    printHexDump(std::begin(image), std::end(image));

                    // TODO: save the firmware image somewhere.
                }
                else
                {
                    std::cout << "Firmware download failed" << std::endl;

                    // TODO: handle the error, e.g. retry download, send a log message, etc.
                }

                fw_loader.destroy();
            }
        }
        else
        {
            node.setModeOperational();
        }

        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(500));
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

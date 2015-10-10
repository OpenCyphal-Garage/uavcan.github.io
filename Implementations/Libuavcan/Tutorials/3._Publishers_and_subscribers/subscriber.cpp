#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan/protocol/debug/LogMessage.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

constexpr unsigned NodeMemoryPoolSize = 16384;
typedef uavcan::Node<NodeMemoryPoolSize> Node;


static Node& getNode()
{
    static Node node(getCanDriver(), getSystemClock());
    return node;
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.subscriber");

    /*
     * Dependent objects (e.g. publishers, subscribers, servers, callers, timers, ...) can be initialized only
     * if the node is running. Note that all dependent objects always keep a reference to the node object.
     */
    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * Subscribing to standard log messages of type uavcan.protocol.debug.LogMessage.
     *
     * Received messages will be passed to the application via a callback, the type of which can be set via the second
     * template argument.
     * In C++11 mode, callback type defaults to std::function<>.
     * In C++03 mode, callback type defaults to a plain function pointer; use a binder object to call member
     * functions as callbacks (refer to uavcan::MethodBinder<>).
     *
     * N.B.: Some libuavcan users report that C++ lambda functions when used with GCC may actually break the code
     *       on some embedded targets, particularly ARM Cortex M0. These reports still remain unconfirmed though;
     *       please refer to the UAVCAN mailing list to learn more.
     *
     * The type of the argument of the callback can be either of these two:
     *  - T&
     *  - uavcan::ReceivedDataStructure<T>&
     * For the first option, ReceivedDataStructure<T>& will be cast into a T& implicitly.
     *
     * The class uavcan::ReceivedDataStructure extends the received data structure with extra information obtained from
     * the transport layer, such as Source Node ID, timestamps, Transfer ID, index of the redundant interface this
     * transfer was picked up from, etc.
     */
    uavcan::Subscriber<uavcan::protocol::debug::LogMessage> log_sub(node);

    const int log_sub_start_res = log_sub.start(
        [&](const uavcan::ReceivedDataStructure<uavcan::protocol::debug::LogMessage>& msg)
        {
            /*
             * The message will be streamed in YAML format.
             */
            std::cout << msg << std::endl;
            /*
             * If the standard iostreams are not available (they rarely available in embedded environments),
             * use the helper class uavcan::OStream defined in the header file <uavcan/helpers/ostream.hpp>.
             */
            // uavcan::OStream::instance() << msg << uavcan::OStream::endl;
        });
    /*
     * C++03 WARNING
     * The code above will not compile in C++03, because it uses a lambda function.
     * In order to compile the code in C++03, move the code from the lambda to a standalone static function.
     * Use uavcan::MethodBinder<> to invoke member functions.
     */

    if (log_sub_start_res < 0)
    {
        throw std::runtime_error("Failed to start the log subscriber; error: " + std::to_string(log_sub_start_res));
    }

    /*
     * Subscribing to messages of type uavcan.protocol.debug.KeyValue.
     * This time we don't want to receive extra information about the received message, so the callback's argument type
     * would be just T& instead of uavcan::ReceivedDataStructure<T>&.
     * The callback will print the message in YAML format via std::cout (also refer to uavcan::OStream).
     */
    uavcan::Subscriber<uavcan::protocol::debug::KeyValue> kv_sub(node);

    const int kv_sub_start_res =
        kv_sub.start([&](const uavcan::protocol::debug::KeyValue& msg) { std::cout << msg << std::endl; });

    if (kv_sub_start_res < 0)
    {
        throw std::runtime_error("Failed to start the key/value subscriber; error: " + std::to_string(kv_sub_start_res));
    }

    /*
     * Running the node.
     */
    node.setModeOperational();

    while (true)
    {
        /*
         * The method spin() may return earlier if an error occurs (e.g. driver failure).
         * All error codes are listed in the header uavcan/error.hpp.
         */
        const int res = node.spin(uavcan::MonotonicDuration::getInfinite());
        if (res < 0)
        {
            std::cerr << "Transient failure: " << res << std::endl;
        }
    }
}

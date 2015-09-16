#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * We're going to use messages of type uavcan.protocol.debug.KeyValue, so the appropriate header must be included.
 * Given a data type named X, the header file name would be:
 *      X.replace('.', '/') + ".hpp"
 */
#include <uavcan/protocol/debug/KeyValue.hpp> // uavcan.protocol.debug.KeyValue


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
        std::printf("Usage: %s <node-id>\n", argv[0]);
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    auto& node = getNode();
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.pubsub");

    /*
     * Dependent objects (e.g. publishers, subscribers, servers, callers, timers, ...) can be initialized only
     * if the node is running. Note that all dependent objects always keep a reference to the node object.
     */
    while (true)
    {
        const int res = node.start();
        if (res < 0)
        {
            std::printf("Node start failed: %d, will retry\n", res);
            ::sleep(1);
        }
        else { break; }
    }

    /*
     * Create the publisher object to broadcast standard key-value messages of type uavcan.protocol.debug.KeyValue.
     * Keep in mind that most classes defined in the library are not copyable; attempt to copy objects of
     * such classes will result in compilation failure.
     */
    uavcan::Publisher<uavcan::protocol::debug::KeyValue> kv_pub(node);
    const int kv_pub_init_res = kv_pub.init();
    if (kv_pub_init_res < 0)
    {
        std::exit(1);                   // TODO proper error handling
    }

    /*
     * This would fail because most of the objects - including publishers - are noncopyable.
     * The error message may look like this:
     *  "error: ‘uavcan::Noncopyable::Noncopyable(const uavcan::Noncopyable&)’ is private"
     */
    // auto pub_copy = kv_pub;  // Don't try this at home.

    /*
     * TX timeout can be overridden if needed.
     * Default value should be OK for most use cases.
     */
    kv_pub.setTxTimeout(uavcan::MonotonicDuration::fromMSec(1000));

    /*
     * Priority of outgoing tranfers can be changed as follows.
     * Default priority is 16 (medium).
     */
    kv_pub.setPriority(uavcan::TransferPriority::MiddleLower);

    /*
     * Subscribing to standard log messages of type uavcan.protocol.debug.LogMessage.
     * Note that the appropriate header is already included in the library headers.
     *
     * Received messages will be passed to the application via a callback, the type of which can be set via the second
     * template argument.
     * In C++11 mode, callback type defaults to std::function<>.
     * In C++03 mode, callback type defaults to a plain function pointer; use a binder object to call member
     * functions as callbacks (refer to uavcan::MethodBinder<>).
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

    if (log_sub_start_res < 0)
    {
        std::exit(1);                   // TODO proper error handling
    }

    /*
     * Subscribing to messages of type uavcan.protocol.debug.KeyValue (same as we're publishing).
     * A publishing node won't see its own messages.
     * This time we don't want to receive extra information about the received message, so the callback's argument type
     * would be just T& instead of uavcan::ReceivedDataStructure<T>&.
     * The callback will print the message in YAML format via std::cout (also refer to uavcan::OStream).
     */
    uavcan::Subscriber<uavcan::protocol::debug::KeyValue> kv_sub(node);

    const int kv_sub_start_res =
        kv_sub.start([&](const uavcan::protocol::debug::KeyValue& msg) { std::cout << msg << std::endl; });

    if (kv_sub_start_res < 0)
    {
        std::exit(1);                   // TODO proper error handling
    }

    /*
     * Running the node.
     */
    node.setModeOperational();

    while (true)
    {
        /*
         * Spinning for 1 second.
         * The method spin() may return earlier if an error occurs (e.g. driver failure).
         * All error codes are listed in the header uavcan/error.hpp.
         */
        const int res = node.spin(uavcan::MonotonicDuration::fromMSec(1000));
        if (res < 0)
        {
            std::printf("Transient failure: %d\n", res);
        }

        /*
         * Publishing a random value using the publisher created above.
         * All message types have zero-initializing default constructors.
         * Relevant usage info for every data type is provided in its DSDL definition.
         */
        uavcan::protocol::debug::KeyValue kv_msg;  // Always zero initialized
        kv_msg.value = std::rand() / float(RAND_MAX);

        /*
         * Arrays in DSDL types are quite extensive in the sense that they can be static,
         * or dynamic (no heap needed - all memory is pre-allocated), or they can emulate std::string.
         * The last one is called string-like arrays.
         * ASCII strings can be directly assigned or appended to string-like arrays.
         * For more info, please read the documentation for the class uavcan::Array<>.
         */
        kv_msg.key = "random";  // "random"
        kv_msg.key += "_";      // "random_"
        kv_msg.key += "float";  // "random_float"

        /*
         * Publishing the message. Two methods are available:
         *  - broadcast(message)
         *  - unicast(message, destination_node_id)
         * Here we use broadcasting.
         */
        const int pub_res = kv_pub.broadcast(kv_msg);
        if (pub_res < 0)
        {
            std::printf("KV publication failure: %d\n", pub_res);
        }
    }
}

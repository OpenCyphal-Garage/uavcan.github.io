#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <uavcan/uavcan.hpp>

/*
 * Implementations for the standard application-level functions are located in uavcan/protocol/.
 * The same path also contains the standard data types uavcan.protocol.*.
 */
#include <uavcan/protocol/param_server.hpp>

extern uavcan::ICanDriver& getCanDriver();
extern uavcan::ISystemClock& getSystemClock();

constexpr unsigned NodeMemoryPoolSize = 16384;

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <node-id>" << std::endl;
        return 1;
    }

    const int self_node_id = std::stoi(argv[1]);

    uavcan::Node<NodeMemoryPoolSize> node(getCanDriver(), getSystemClock());
    node.setNodeID(self_node_id);
    node.setName("org.uavcan.tutorial.configuree");

    const int node_start_res = node.start();
    if (node_start_res < 0)
    {
        throw std::runtime_error("Failed to start the node; error: " + std::to_string(node_start_res));
    }

    /*
     * This would be our configuration storage.
     */
    static struct Params
    {
        unsigned foo = 42;
        float bar = 0.123456F;
        double baz = 1e-5;
        std::string booz = "Hello world!";
    } configuration;

    /*
     * Now, we need to define some glue logic between the server (below) and our configuration storage (above).
     * This is done via the interface uavcan::IParamManager.
     */
    class : public uavcan::IParamManager
    {
        void getParamNameByIndex(Index index, Name& out_name) const override
        {
            if (index == 0) { out_name = "foo"; }
            if (index == 1) { out_name = "bar"; }
            if (index == 2) { out_name = "baz"; }
            if (index == 3) { out_name = "booz"; }
        }

        void assignParamValue(const Name& name, const Value& value) override
        {
            if (name == "foo")
            {
                /*
                 * Parameter "foo" is an integer, so we accept only integer values here.
                 */
                if (value.is(uavcan::protocol::param::Value::Tag::integer_value))
                {
                    configuration.foo = *value.as<uavcan::protocol::param::Value::Tag::integer_value>();
                }
            }
            else if (name == "bar")
            {
                /*
                 * Parameter "bar" is a floating point, so we accept only float values here.
                 */
                if (value.is(uavcan::protocol::param::Value::Tag::real_value))
                {
                    configuration.bar = *value.as<uavcan::protocol::param::Value::Tag::real_value>();
                }
            }
            else if (name == "baz")
            {
                /*
                 * Ditto
                 */
                if (value.is(uavcan::protocol::param::Value::Tag::real_value))
                {
                    configuration.baz = *value.as<uavcan::protocol::param::Value::Tag::real_value>();
                }
            }
            else if (name == "booz")
            {
                /*
                 * Parameter "booz" is a string, so we take only strings.
                 */
                if (value.is(uavcan::protocol::param::Value::Tag::string_value))
                {
                    configuration.booz = value.as<uavcan::protocol::param::Value::Tag::string_value>()->c_str();
                }
            }
            else
            {
                std::cout << "Can't assign parameter: " << name.c_str() << std::endl;
            }
        }

        void readParamValue(const Name& name, Value& out_value) const override
        {
            if (name == "foo")
            {
                out_value.to<uavcan::protocol::param::Value::Tag::integer_value>() = configuration.foo;
            }
            else if (name == "bar")
            {
                out_value.to<uavcan::protocol::param::Value::Tag::real_value>() = configuration.bar;
            }
            else if (name == "baz")
            {
                out_value.to<uavcan::protocol::param::Value::Tag::real_value>() = configuration.baz;
            }
            else if (name == "booz")
            {
                out_value.to<uavcan::protocol::param::Value::Tag::string_value>() = configuration.booz.c_str();
            }
            else
            {
                std::cout << "Can't read parameter: " << name.c_str() << std::endl;
            }
        }

        int saveAllParams() override
        {
            std::cout << "Save - this implementation does not require any action" << std::endl;
            return 0;     // Zero means that everything is fine.
        }

        int eraseAllParams() override
        {
            std::cout << "Erase - all params reset to default values" << std::endl;
            configuration = Params();
            return 0;
        }

        /**
         * Note that this method is optional. It can be left unimplemented.
         */
        void readParamDefaultMaxMin(const Name& name, Value& out_def,
                                    NumericValue& out_max, NumericValue& out_min) const override
        {
            if (name == "foo")
            {
                out_def.to<uavcan::protocol::param::Value::Tag::integer_value>() = Params().foo;
                out_max.to<uavcan::protocol::param::NumericValue::Tag::integer_value>() = 9000;
                out_min.to<uavcan::protocol::param::NumericValue::Tag::integer_value>() = 0;
            }
            else if (name == "bar")
            {
                out_def.to<uavcan::protocol::param::Value::Tag::real_value>() = Params().bar;
                out_max.to<uavcan::protocol::param::NumericValue::Tag::real_value>() = 1;
                out_min.to<uavcan::protocol::param::NumericValue::Tag::real_value>() = 0;
            }
            else if (name == "baz")
            {
                out_def.to<uavcan::protocol::param::Value::Tag::real_value>() = Params().baz;
                out_max.to<uavcan::protocol::param::NumericValue::Tag::real_value>() = 1;
                out_min.to<uavcan::protocol::param::NumericValue::Tag::real_value>() = 0;
            }
            else if (name == "booz")
            {
                out_def.to<uavcan::protocol::param::Value::Tag::string_value>() = Params().booz.c_str();
                std::cout << "Limits for 'booz' are not defined" << std::endl;
            }
            else
            {
                std::cout << "Can't read the limits for parameter: " << name.c_str() << std::endl;
            }
        }
    } param_manager;

    /*
     * Initializing the configuration server.
     * A pointer to the glue logic object is passed to the method start().
     */
    uavcan::ParamServer server(node);

    const int server_start_res = server.start(&param_manager);
    if (server_start_res < 0)
    {
        throw std::runtime_error("Failed to start ParamServer: " + std::to_string(server_start_res));
    }

    /*
     * Now, this node can be reconfigured via UAVCAN. Awesome.
     * Many embedded applications require a restart before the new configuration settings can
     * be applied, so it is highly recommended to also support the remote restart service.
     */
    class : public uavcan::IRestartRequestHandler
    {
        bool handleRestartRequest(uavcan::NodeID request_source) override
        {
            std::cout << "Got a remote restart request from " << int(request_source.get()) << std::endl;
            /*
             * We won't really restart, so return 'false'.
             * Returning 'true' would mean that we're going to restart for real.
             * Note that it is recognized that some nodes may not be able to respond to the
             * restart request (e.g. if they restart immediately from the service callback).
             */
            return false;
        }
    } restart_request_handler;

    /*
     * Installing the restart request handler.
     */
    node.setRestartRequestHandler(&restart_request_handler); // Done.

    /*
     * Running the node.
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

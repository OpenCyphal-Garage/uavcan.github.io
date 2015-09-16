#include <uavcan_lpc11c24/uavcan_lpc11c24.hpp>

static constexpr std::uint32_t BitRate = 1000000;

uavcan::ISystemClock& getSystemClock()
{
    return uavcan_lpc11c24::SystemClock::instance();
}

uavcan::ICanDriver& getCanDriver()
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        int res = uavcan_lpc11c24::CanDriver::instance().init(BitRate);
        if (res < 0)
        {
            // Handle the error
        }
    }
    return uavcan_lpc11c24::CanDriver::instance();
}

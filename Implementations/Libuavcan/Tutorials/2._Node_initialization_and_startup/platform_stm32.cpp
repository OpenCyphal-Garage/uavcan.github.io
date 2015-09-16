#include <uavcan_stm32/uavcan_stm32.hpp>

static constexpr int RxQueueSize = 64;
static constexpr std::uint32_t BitRate = 1000000;

uavcan::ISystemClock& getSystemClock()
{
    return uavcan_stm32::SystemClock::instance();
}

uavcan::ICanDriver& getCanDriver()
{
    static uavcan_stm32::CanInitHelper<RxQueueSize> can;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        int res = can.init(BitRate);
        if (res < 0)
        {
            // Handle the error
        }
    }
    return can.driver;
}

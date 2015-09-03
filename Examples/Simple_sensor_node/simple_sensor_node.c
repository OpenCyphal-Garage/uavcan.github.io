/*
 * This program is an example implementation of an extremely lightweight UAVCAN node with zero third-party dependencies.
 * This example is based on Linux SocketCAN, but can be easily adapted to any other platform.
 *
 * GCC invocation command:
 *     gcc simple_sensor_node.c -lrt -std=gnu99 -o simple_sensor_node
 * With warnings:
 *     gcc simple_sensor_node.c -lrt -std=gnu99 -o simple_sensor_node -Wall -Werror -Wextra -pedantic -Wsign-conversion
 *
 * Author: Pavel Kirienko <pavel.kirienko@zubax.com>
 * License: CC0, no copyright reserved
 * Language: C99
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * This part is specific for Linux. It can be replaced for use on other platforms.
 * Note that under Linux this source must be linked with librt (for GCC add -lrt)
 */
#include <sys/time.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <time.h>

static int can_socket = -1;

int can_init(const char* can_iface_name)
{
    // Open the SocketCAN socket
    const int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0)
    {
        return -1;
    }

    // Resolve the iface index
    struct ifreq ifr;
    (void)memset(&ifr, 0, sizeof(ifr));
    (void)strncpy(ifr.ifr_name, can_iface_name, IFNAMSIZ);
    const int ioctl_result = ioctl(sock, SIOCGIFINDEX, &ifr);
    if (ioctl_result < 0)
    {
        return -1;
    }

    // Assign the iface
    struct sockaddr_can addr;
    (void)memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind the socket to the iface
    const int bind_result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (bind_result < 0)
    {
        return -1;
    }

    can_socket = sock;
    return 0;
}

int can_send(uint32_t extended_can_id, const uint8_t* frame_data, uint8_t frame_data_len)
{
    if (frame_data_len > 8 || frame_data == NULL)
    {
        return -1;
    }

    struct can_frame frame;
    frame.can_id  = extended_can_id | CAN_EFF_FLAG;
    frame.can_dlc = frame_data_len;
    memcpy(frame.data, frame_data, frame_data_len);

    return write(can_socket, &frame, sizeof(struct can_frame));
}

uint64_t get_monotonic_usec(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000UL;
}
/*
 * End of the platform-specific part
 */

/*
 * UAVCAN transport layer
 */
/// Arbitrary priority values
static const uint8_t PRIORITY_HIGHEST = 0;
static const uint8_t PRIORITY_HIGH    = 8;
static const uint8_t PRIORITY_MEDIUM  = 16;
static const uint8_t PRIORITY_LOW     = 24;
static const uint8_t PRIORITY_LOWEST  = 31;

static uint8_t uavcan_node_id;

int uavcan_broadcast(uint8_t priority,
                     uint16_t data_type_id,
                     uint8_t transfer_id,
                     const uint8_t* payload,
                     uint16_t payload_len)
{
    if (payload == NULL || payload_len > 7)
    {
        return -1;  // In this super-simple implementation we don't support multi-frame transfers.
    }

    if (priority > 31)
    {
        return -1;
    }

    const uint32_t can_id = ((uint32_t)priority << 24) | ((uint32_t)data_type_id << 8) | (uint32_t)uavcan_node_id;

    uint8_t payload_with_tail_byte[8];

    memcpy(payload_with_tail_byte, payload, payload_len);

    payload_with_tail_byte[payload_len] = 0xC0 | (transfer_id & 31);

    return can_send(can_id, payload_with_tail_byte, payload_len + 1);
}

/*
 * Float16 support
 */
uint16_t make_float16(float value)
{
    union fp32
    {
        uint32_t u;
        float f;
    };

    const union fp32 f32infty = { 255U << 23 };
    const union fp32 f16infty = { 31U << 23 };
    const union fp32 magic = { 15U << 23 };
    const uint32_t sign_mask = 0x80000000U;
    const uint32_t round_mask = ~0xFFFU;

    union fp32 in;
    uint16_t out = 0;

    in.f = value;

    uint32_t sign = in.u & sign_mask;
    in.u ^= sign;

    if (in.u >= f32infty.u)
    {
        out = (in.u > f32infty.u) ? 0x7FFFU : 0x7C00U;
    }
    else
    {
        in.u &= round_mask;
        in.f *= magic.f;
        in.u -= round_mask;
        if (in.u > f16infty.u)
        {
            in.u = f16infty.u;
        }
        out = (uint16_t)(in.u >> 13);
    }

    out |= (uint16_t)(sign >> 16);

    return out;
}

/*
 * Application logic
 */
/// Defined for the standard data type uavcan.protocol.NodeStatus
enum node_health
{
    HEALTH_OK       = 0,
    HEALTH_WARNING  = 1,
    HEALTH_ERROR    = 2,
    HEALTH_CRITICAL = 3
};

/// Defined for the standard data type uavcan.protocol.NodeStatus
enum node_mode
{
    MODE_OPERATIONAL     = 0,
    MODE_INITIALIZATION  = 1,
    MODE_MAINTENANCE     = 2,
    MODE_SOFTWARE_UPDATE = 3,
    MODE_OFFLINE         = 7
};

/// Standard data type: uavcan.protocol.NodeStatus
int publish_node_status(enum node_health health, enum node_mode mode, uint16_t vendor_specific_status_code)
{
    static uint64_t startup_timestamp_usec;
    if (startup_timestamp_usec == 0)
    {
        startup_timestamp_usec = get_monotonic_usec();
    }

    uint8_t payload[7];

    // Uptime in seconds
    const uint32_t uptime_sec = (get_monotonic_usec() - startup_timestamp_usec) / 1000000ULL;
    payload[0] = (uptime_sec >> 0)  & 0xFF;
    payload[1] = (uptime_sec >> 8)  & 0xFF;
    payload[2] = (uptime_sec >> 16) & 0xFF;
    payload[3] = (uptime_sec >> 24) & 0xFF;

    // Health and mode
    payload[4] = ((uint8_t)health << 6) | ((uint8_t)mode << 3);

    // Vendor-specific status code
    payload[5] = (vendor_specific_status_code >> 0) & 0xFF;
    payload[6] = (vendor_specific_status_code >> 8) & 0xFF;

    static const uint16_t data_type_id = 341;
    static uint8_t transfer_id;

    return uavcan_broadcast(PRIORITY_LOW, data_type_id, transfer_id++, payload, sizeof(payload));
}

/// Standard data type: uavcan.equipment.air_data.TrueAirspeed
int publish_true_airspeed(float mean, float variance)
{
    const uint16_t f16_mean     = make_float16(mean);
    const uint16_t f16_variance = make_float16(variance);

    uint8_t payload[4];
    payload[0] = (f16_mean >> 0) & 0xFF;
    payload[1] = (f16_mean >> 8) & 0xFF;
    payload[2] = (f16_variance >> 0) & 0xFF;
    payload[3] = (f16_variance >> 8) & 0xFF;

    static const uint16_t data_type_id = 1020;
    static uint8_t transfer_id;
    transfer_id += 1;
    return uavcan_broadcast(PRIORITY_MEDIUM, data_type_id, transfer_id, payload, sizeof(payload));
}

int compute_true_airspeed(float* out_airspeed, float* out_variance)
{
    *out_airspeed = 1.2345F; // This is a stub.
    *out_variance = 0.0F;    // By convention, zero represents unknown error variance.
    return 0;
}

int main(int argc, char** argv)
{
    /*
     * Node initialization
     */
    if (argc < 3)
    {
        puts("Args: <self-node-id> <can-iface-name>");
        return 1;
    }

    uavcan_node_id = (uint8_t)atoi(argv[1]);
    if (uavcan_node_id < 1 || uavcan_node_id > 127)
    {
        printf("%i is not a valid node ID\n", (int)uavcan_node_id);
        return 1;
    }

    if (can_init(argv[2]) < 0)
    {
        printf("Failed to open iface %s\n", argv[2]);
        return 1;
    }

    /*
     * Main loop
     */
    enum node_health health = HEALTH_OK;

    for (;;)
    {
        float airspeed = 0.0F;
        float airspeed_variance = 0.0F;
        const int airspeed_computation_result = compute_true_airspeed(&airspeed, &airspeed_variance);

        if (airspeed_computation_result == 0)
        {
            const int publication_result = publish_true_airspeed(airspeed, airspeed_variance);
            health = (publication_result < 0) ? HEALTH_ERROR : HEALTH_OK;
        }
        else
        {
            health = HEALTH_ERROR;
        }

        const uint16_t vendor_specific_status_code = rand(); // Can be used to report vendor-specific status info

        (void)publish_node_status(health, MODE_OPERATIONAL, vendor_specific_status_code);

        usleep(500000);
    }
}

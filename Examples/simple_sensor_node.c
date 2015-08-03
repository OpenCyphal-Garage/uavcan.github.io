/*
 * This program is an example implementation of an extremely lightweight UAVCAN node with zero third-party dependencies.
 * This example is based on Linux SocketCAN, but can be easily adapted to any other platform.
 * Compiler invocation command:
 *     gcc simple_sensor_node.c -lrt -std=gnu99
 *
 * Author: Pavel Kirienko <pavel.kirienko@zubax.com>
 * License: Public domain
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
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000UL;
}
/*
 * End of the platform-specific part
 */

/*
 * UAVCAN transport layer
 */
static uint8_t uavcan_node_id;

int uavcan_broadcast(uint16_t data_type_id, uint8_t transfer_id, const uint8_t* payload, uint16_t payload_len)
{
    if (payload == NULL || payload_len > 8)
    {
        return -1;  // In this super-simple implementation we don't support multi-frame transfers.
    }

    const uint32_t can_id =
        (transfer_id & 7) |              // Transfer ID
        (1 << 3) |                       // Last Frame (always set)
        (0 << 4) |                       // Frame Index (always 0)
        ((uavcan_node_id & 127) << 10) | // Source Node ID
        (2 << 17) |                      // Transfer Type (always 2 - Message Broadcast)
        ((data_type_id & 1023) << 19);   // Data Type ID

    return can_send(can_id, payload, payload_len);
}

/*
 * Float16 support
 */
uint16_t make_float16(float value)
{
    uint16_t hbits = signbit(value) << 15;
    if (value == 0.0F)
    {
        return hbits;
    }
    if (isnan(value))
    {
        return hbits | 0x7FFFU;
    }
    if (isinf(value))
    {
        return hbits | 0x7C00U;
    }
    int exp;
    (void)frexp(value, &exp);
    if (exp > 16)
    {
        return hbits | 0x7C00U;
    }
    if (exp < -13)
    {
        value = ldexp(value, 24);
    }
    else
    {
        value = ldexp(value, 11 - exp);
        hbits |= ((exp + 14) << 10);
    }
    const int32_t ival = (int32_t)value;
    hbits |= (uint16_t)(((ival < 0) ? (-ival) : ival) & 0x3FFU);
    float diff = fabs(value - (float)ival);
    hbits += diff >= 0.5F;
    return hbits;
}

/*
 * Application logic
 */
 // These constants are defined for the standard data type uavcan.protocol.NodeStatus
enum node_status
{
    STATUS_OK           = 0,
    STATUS_INITIALIZING = 1,
    STATUS_WARNING      = 2,
    STATUS_CRITICAL     = 3,
    STATUS_OFFLINE      = 15
};

/// Standard data type: uavcan.protocol.NodeStatus
int publish_node_status(enum node_status node_status, uint16_t vendor_specific_status_code)
{
    static uint64_t startup_timestamp_usec;
    if (startup_timestamp_usec == 0)
    {
        startup_timestamp_usec = get_monotonic_usec();
    }

    uint8_t payload[6];

    // Uptime in seconds - first 28 bits of the message
    const uint32_t uptime_sec = (get_monotonic_usec() - startup_timestamp_usec) / 1000000ULL;
    payload[0] = (uptime_sec >> 0)  & 0xFF;
    payload[1] = (uptime_sec >> 8)  & 0xFF;
    payload[2] = (uptime_sec >> 16) & 0xFF;
    payload[3] = ((uptime_sec >> 24) & 0x0F) << 4;

    // Status - next 4 bits
    payload[3] |= (uint8_t)node_status & 0x0F;

    // Vendor-specific status code
    payload[4] = (vendor_specific_status_code >> 0) & 0xFF;
    payload[5] = (vendor_specific_status_code >> 8) & 0xFF;

    static const uint16_t data_type_id = 550;
    static uint8_t transfer_id;
    transfer_id += 1;
    return uavcan_broadcast(data_type_id, transfer_id, payload, sizeof(payload));
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

    static const uint16_t data_type_id = 290;
    static uint8_t transfer_id;
    transfer_id += 1;
    return uavcan_broadcast(data_type_id, transfer_id, payload, sizeof(payload));
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
    enum node_status node_status = STATUS_INITIALIZING;

    for (;;)
    {
        float airspeed = 0.0F;
        float airspeed_variance = 0.0F;
        const int airspeed_computation_result = compute_true_airspeed(&airspeed, &airspeed_variance);

        if (airspeed_computation_result == 0)
        {
            const int publication_result = publish_true_airspeed(airspeed, airspeed_variance);
            node_status = (publication_result < 0) ? STATUS_WARNING : STATUS_OK;
        }
        else
        {
            node_status = STATUS_WARNING;
        }

        const uint16_t vendor_specific_status_code = rand(); // Can be used to report vendor-specific status info

        (void)publish_node_status(node_status, vendor_specific_status_code);

        usleep(500000);
    }
}

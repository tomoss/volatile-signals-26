#ifndef DEVICE_HEALTH_HPP
#define DEVICE_HEALTH_HPP

#include <cstdint>
#include <ctime>

struct DeviceHealth {
    int8_t rssi = 0;
    uint32_t heap = 0;
    uint32_t minHeap = 0;
    uint32_t uptime = 0;
    time_t timestamp = 0;
};

#endif // DEVICE_HEALTH_HPP

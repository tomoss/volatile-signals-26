#ifndef DEVICE_INFO_HPP
#define DEVICE_INFO_HPP

#include <cstdint>

struct DeviceInfo {
    const char* firmwareVersion = nullptr;
    const char* chipModel = nullptr;
    uint16_t chipRevision = 0; // ESP.getChipRevision() encodes major/minor (e.g. 100 = v1.0)
    uint8_t chipCores = 0;
    uint8_t resetReason = 0;
    uint32_t totalHeap = 0; // ESP.getHeapSize() - total heap capacity in bytes, denominator for heap% elsewhere
};

#endif // DEVICE_INFO_HPP

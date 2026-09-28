#ifndef DEVICE_INFO_HPP
#define DEVICE_INFO_HPP

#include <cstdint>

#include "00_vendor/arduino.hpp"

#include <esp_system.h>

struct DeviceInfo {
    const char* firmwareVersion = nullptr;
    const char* chipModel = nullptr;
    uint16_t chipRevision = 0; // ESP.getChipRevision() encodes major/minor (e.g. 100 = v1.0)
    uint8_t chipCores = 0;
    uint8_t resetReason = 0;
    uint32_t totalHeap = 0; // ESP.getHeapSize() - total heap capacity in bytes, denominator for heap% elsewhere
};

// Reads the chip/firmware facts above off the running hardware. Values don't change at
// runtime, so this is meant to be called once, in setup().
inline DeviceInfo collectDeviceInfo() {
    DeviceInfo l_info;
    l_info.firmwareVersion = FIRMWARE_VERSION;
    l_info.chipModel = ESP.getChipModel();
    l_info.chipRevision = ESP.getChipRevision();
    l_info.chipCores = ESP.getChipCores();
    l_info.resetReason = static_cast<uint8_t>(esp_reset_reason());
    l_info.totalHeap = ESP.getHeapSize();
    return l_info;
}

#endif // DEVICE_INFO_HPP

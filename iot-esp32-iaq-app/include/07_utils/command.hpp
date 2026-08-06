#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string_view>

enum class Command : uint8_t { DeviceReboot = 0, SensorLowPower = 1, SensorUltraLowPower = 2, DeviceClaimed = 3, DeviceUnclaimed = 4, Unknown = 5 };

inline Command parseCommand(std::string_view p_cmd) {
    if (p_cmd == "device_reboot") {
        return Command::DeviceReboot;
    }

    if (p_cmd == "sensor_lp") {
        return Command::SensorLowPower;
    }

    if (p_cmd == "sensor_ulp") {
        return Command::SensorUltraLowPower;
    }

    if (p_cmd == "device_claimed") {
        return Command::DeviceClaimed;
    }

    if (p_cmd == "device_unclaimed") {
        return Command::DeviceUnclaimed;
    }

    return Command::Unknown;
}

#endif // COMMAND_HPP
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string_view>

enum class Command : uint8_t { Reboot = 0, SensorLowPower = 1, SensorUltraLowPower = 2, Unknown = 3 };

inline Command parseCommand(std::string_view p_cmd) {
    if (p_cmd == "reboot") {
        return Command::Reboot;
    }

    if (p_cmd == "sensor_lp") {
        return Command::SensorLowPower;
    }

    if (p_cmd == "sensor_ulp") {
        return Command::SensorUltraLowPower;
    }

    return Command::Unknown;
}

#endif // COMMAND_HPP

#ifndef DISPLAY_TYPES_HPP
#define DISPLAY_TYPES_HPP

#include <cstdint>

#include "07_utils/claim_code.hpp"

// Which full-screen overlay, if any, currently takes precedence over the normal WiFi/MQTT/env
// status screen. At most one is shown at a time.
enum class DisplayOverlay : uint8_t {
    None,
    Provisioning,
    Claim,
};

struct DisplayState {
    DisplayOverlay overlay{DisplayOverlay::None};
    uint32_t provisionPasskey{0};
    ClaimCode claimCode{};
    bool claimed{false};
    bool wifiConnected{false};
    bool mqttConnected{false};
    uint16_t iaq{0};
    int8_t temperatureC{0};
    uint8_t accuracy{0};
};

#endif // DISPLAY_TYPES_HPP

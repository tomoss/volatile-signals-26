#ifndef DISPLAY_TYPES_HPP
#define DISPLAY_TYPES_HPP

#include <cstdint>

#include "07_utils/claim_code.hpp"

struct ProvisionDisplayState {
    bool active{false};
    uint32_t passkey{0};

    bool operator==(const ProvisionDisplayState& other) const { return active == other.active && passkey == other.passkey; }
};

struct ClaimingDisplayState {
    bool active{false};
    ClaimCode code{};

    bool operator==(const ClaimingDisplayState& other) const { return active == other.active && code == other.code; }
};

// Shown on a button press once the device is already registered, in place of the claim code.
struct AlreadyClaimedDisplayState {
    bool active{false};

    bool operator==(const AlreadyClaimedDisplayState& other) const { return active == other.active; }
};

struct WifiDisplayState {
    bool connected{false};

    bool operator==(const WifiDisplayState& other) const { return connected == other.connected; }
};

struct MqttDisplayState {
    bool connected{false};

    bool operator==(const MqttDisplayState& other) const { return connected == other.connected; }
};

struct EnvDisplayState {
    uint16_t iaq{0};
    int8_t temperatureC{0};
    uint8_t accuracy{0};

    bool operator==(const EnvDisplayState& other) const {
        return iaq == other.iaq && temperatureC == other.temperatureC && accuracy == other.accuracy;
    }
};

struct DisplayState {
    ProvisionDisplayState provision;
    ClaimingDisplayState claiming;
    AlreadyClaimedDisplayState alreadyClaimed;
    WifiDisplayState wifi;
    MqttDisplayState mqtt;
    EnvDisplayState env;
};

#endif // DISPLAY_TYPES_HPP
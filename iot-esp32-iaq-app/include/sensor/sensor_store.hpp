#ifndef SENSOR_STORE_HPP
#define SENSOR_STORE_HPP

#include <array>
#include <cstdint>
#include <optional>

#include "sensor/sensor_types.hpp"
#include "vendor/bsec2.hpp"

using SensorState = std::array<uint8_t, BSEC_MAX_STATE_BLOB_SIZE>;

class SensorStore {
public:
    virtual ~SensorStore() = default;

    virtual std::optional<SensorState> loadBsecState(SensorMode p_mode) = 0;
    virtual bool saveBsecState(SensorMode p_mode, const SensorState& p_state) = 0;

    virtual std::optional<SensorMode> loadSensorMode() = 0;
    virtual bool saveSensorMode(SensorMode p_mode) = 0;
};

#endif // SENSOR_STORE_HPP

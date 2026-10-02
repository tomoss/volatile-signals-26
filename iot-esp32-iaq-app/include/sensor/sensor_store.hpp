#ifndef SENSOR_STORE_HPP
#define SENSOR_STORE_HPP

#include <optional>

#include "sensor/sensor_state.hpp"
#include "sensor/sensor_types.hpp"

class SensorStore {
public:
    virtual ~SensorStore() = default;

    virtual std::optional<SensorState> loadBsecState(SensorMode p_mode) = 0;
    virtual bool saveBsecState(SensorMode p_mode, const SensorState& p_state) = 0;

    virtual std::optional<SensorMode> loadSensorMode() = 0;
    virtual bool saveSensorMode(SensorMode p_mode) = 0;
};

#endif // SENSOR_STORE_HPP

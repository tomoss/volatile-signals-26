#ifndef ENV_SENSOR_H
#define ENV_SENSOR_H

#include <optional>

#include <bsec2.h>
#include <freertos/queue.h>

#include "01_sensor/sensor_types.h"
#include "02_storage/storage.h"

class EnvSensor {
public:
    EnvSensor(Storage& p_storage)
        : m_storage(p_storage) {}

    ~EnvSensor() = default;
    EnvSensor(const EnvSensor&) = delete;
    const EnvSensor& operator=(const EnvSensor&) = delete;
    EnvSensor(EnvSensor&&) = delete;
    EnvSensor& operator=(EnvSensor&&) = delete;

    [[nodiscard]] bool init(SensorMode p_mode = SensorMode::LowPower);
    void run();

    bool setMode(SensorMode p_mode);
    SensorMode getMode() const { return m_mode; }

    QueueHandle_t getQueue() const;

    std::optional<SensorState> getBsecState();
    bool setBsecState(const SensorState& p_state);

    void maybeSaveStateToStorage();

private:
    void checkBsecStatus();

    Bsec2 m_bsec;
    SensorMode m_mode = SensorMode::LowPower;
    bool m_hasSavedStateForMode{false};
    uint64_t m_lastStateSaveMs = 0ULL;
    Storage& m_storage;
};

#endif // ENV_SENSOR_H
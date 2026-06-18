#ifndef ENV_SENSOR_H
#define ENV_SENSOR_H

#include <optional>

#include "00_vendor/bsec2.h"
#include "00_vendor/freertos.h"

#include "01_sensor/sensor_types.h"
#include "02_storage/storage.h"

class EnvSensor {
public:
    EnvSensor(Storage& p_storage)
        : m_storage(p_storage) {}

    ~EnvSensor();
    EnvSensor(const EnvSensor&) = delete;
    const EnvSensor& operator=(const EnvSensor&) = delete;
    EnvSensor(EnvSensor&&) = delete;
    EnvSensor& operator=(EnvSensor&&) = delete;

    [[nodiscard]] bool init(SensorMode p_mode = SensorMode::LowPower);

    // Starts the background task that owns run()/maybeSaveStateToStorage(), call once, after a successful init().
    void begin();

    bool setMode(SensorMode p_mode);
    SensorMode getMode() const { return m_mode; }

    // Thread-safe: queues a mode change to be applied on the next run() call (which always
    // executes on the task that owns this EnvSensor), so callers on other tasks (e.g. the
    // MQTT message callback) never touch m_bsec/m_mode directly.
    bool requestModeChange(SensorMode p_mode);

    QueueHandle_t getQueue() const;

    std::optional<SensorState> getBsecState();
    bool setBsecState(const SensorState& p_state);

private:
    void run();
    void maybeSaveStateToStorage();
    void checkBsecStatus();
    void printMode();

    static void taskEntry(void* p_parameter);
    void taskLoop();

    Bsec2 m_bsec;
    SensorMode m_mode = SensorMode::LowPower;
    bool m_hasSavedStateForMode{false};
    uint64_t m_lastStateSaveMs = 0ULL;
    Storage& m_storage;
    QueueHandle_t m_modeRequestQueue = nullptr;
    TaskHandle_t m_task = nullptr;
};

#endif // ENV_SENSOR_H
#ifndef ENV_SENSOR_HPP
#define ENV_SENSOR_HPP

#include <optional>

#include "00_vendor/bsec2.hpp"
#include "00_vendor/freertos.hpp"
#include "01_sensor/sensor_types.hpp"
#include "02_storage/storage.hpp"
#include "09_utils/task.hpp"
#include "09_utils/wire_wrapper.hpp"

class EnvSensor {
public:
    explicit EnvSensor(Storage& p_storage) : m_storage(p_storage) {}
    ~EnvSensor() = default;
    EnvSensor(const EnvSensor&) = delete;
    const EnvSensor& operator=(const EnvSensor&) = delete;
    EnvSensor(EnvSensor&&) = delete;
    EnvSensor& operator=(EnvSensor&&) = delete;

    [[nodiscard]] bool init(WireWrapper& p_bus);

    // Starts the background task
    void start();

    // Thread-safe: queues a mode change to be applied on the next run() call
    bool requestModeChange(SensorMode p_mode);

    void setConsumerQueue(QueueHandle_t p_consumerQueue) { s_consumerQueue = p_consumerQueue; }

private:
    std::optional<SensorState> getStateFromBsec();
    // set the BME688 sensor state to BSEC lib
    bool setStateToBsec(const SensorState& p_state);

    bool setMode(SensorMode p_mode);
    SensorMode getMode() const { return m_mode; }

    // Applies config/subscription/state-restore for p_mode and updates m_mode. Shared by init() and setMode().
    bool applyMode(SensorMode p_mode);

    void run();
    void checkModeChangeRequest();

    // Save state to storage once accuracy first reaches High for this mode, then every STATE_SAVE_PERIOD_MS as long as
    // accuracy remains High
    void maybeSaveStateToStorage();

    void checkBsecStatus();
    void printMode();

    void loop();

    Bsec2 m_bsec;
    SensorMode m_mode = SensorMode::LowPower;
    bool m_hasSavedStateForMode{false};
    uint64_t m_lastStateSaveMs = 0ULL;
    Storage& m_storage;
    QueueHandle_t m_modeRequestQueue = nullptr;
    Task m_task;

    // Set from setQueue()'s p_consumerQueue. Static because Bsec2::attachCallback only takes a
    // plain (capture-less) function pointer, which can't reach an instance member.
    static QueueHandle_t s_consumerQueue;
};

#endif // ENV_SENSOR_HPP
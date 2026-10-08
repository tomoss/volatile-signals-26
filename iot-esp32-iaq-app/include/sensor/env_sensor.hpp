#ifndef ENV_SENSOR_HPP
#define ENV_SENSOR_HPP

#include <optional>

#include "queue/freertos_queue.hpp"
#include "sensor/sensor_store.hpp"
#include "sensor/sensor_types.hpp"
#include "task/task.hpp"
#include "telemetry/telemetry_sink.hpp"
#include "utils/wire_wrapper.hpp"
#include "vendor/bsec2.hpp"
#include "vendor/freertos.hpp"

// Length 1 so a newer request overwrites one not yet applied
using SensorModeRequestQueue = FreeRtosQueue<SensorMode, 1>;

template<TaskLike TTask>
class EnvSensor {
public:
    EnvSensor(SensorStore& p_store, TelemetrySink& p_telemetrySink) : m_store(p_store) { s_telemetrySink = &p_telemetrySink; }
    ~EnvSensor() = default;
    EnvSensor(const EnvSensor&) = delete;
    const EnvSensor& operator=(const EnvSensor&) = delete;
    EnvSensor(EnvSensor&&) = delete;
    EnvSensor& operator=(EnvSensor&&) = delete;

    [[nodiscard]] bool init(WireWrapper& p_bus);

    // Starts the background task
    void start();

    // Thread-safe: queues a mode change to be applied on the next run() call
    void enqueueModeChange(SensorMode p_mode);

private:
    std::optional<SensorState> getStateFromBsec();
    bool setStateToBsec(const SensorState& p_state);

    bool setMode(SensorMode p_mode);
    // Applies config/subscription/state-restore for p_mode and updates m_mode.
    bool applyMode(SensorMode p_mode);

    // Picks the bundled AI config for p_mode and sets it on m_bsec.
    bool setConfig(SensorMode p_mode);

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
    SensorStore& m_store;
    SensorModeRequestQueue m_modeRequestQueue;
    TTask m_task;

    // Static because Bsec2::attachCallback only takes a plain function pointer
    static inline TelemetrySink* s_telemetrySink = nullptr;
};

#endif // ENV_SENSOR_HPP
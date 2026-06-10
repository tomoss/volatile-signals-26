#include "01_sensor/env_sensor.h"

#include "01_sensor/sensor_data.h"
#include "02_storage/storage.h"
#include "04_utils/mapping_handler.h"

#include <Arduino.h>

constexpr uint64_t STATE_SAVE_PERIOD_MS = 4ULL * 60ULL * 60ULL * 1000ULL; // 4 hours

/* Temperature offset for BME688 sensor - measured with a calibrated thermometer */
constexpr float BME68X_TEMPERATURE_OFFSET = 1.5f;
/* Static queue handle for sensor data because it needs to be accessible from the callback function */
static QueueHandle_t s_sensorQueue = nullptr;
/* The call will return immediately if the queue is full and xTicksToWait is set to 0. */
constexpr const TickType_t TICKS_TO_WAIT = 0;
constexpr const int QUEUE_SIZE = 10;

/* AI configurations for BME688, one per supported sample rate. Each must match the rate
   requested via updateSubscription() below; otherwise BSEC runs on a mismatched config
   and reports BSEC_W_SU_SAMPLERATEMISMATCH (warning 14). */
constexpr const uint8_t s_bsecConfigLp[] = {
#include "config/bme688/bme688_sel_33v_3s_4d/bsec_selectivity.txt"
};

constexpr const uint8_t s_bsecConfigUlp[] = {
#include "config/bme688/bme688_sel_33v_300s_4d/bsec_selectivity.txt"
};

static const uint8_t* modeToConfig(const SensorMode p_mode) {
    switch (p_mode) {
    case SensorMode::UltraLowPower:
        return s_bsecConfigUlp;
    case SensorMode::LowPower:
        return s_bsecConfigLp;
    case SensorMode::Continuous:
        Serial.println("No bundled BSEC config matches BSEC_SAMPLE_RATE_CONT; the LP config is the closest fit");
        return s_bsecConfigLp;
    default:
        /* Let the LowPower config be used as a fallback */
        return s_bsecConfigLp;
    }
}

static float modeToSampleRate(const SensorMode p_mode) {
    switch (p_mode) {
    case SensorMode::UltraLowPower:
        return BSEC_SAMPLE_RATE_ULP;
    case SensorMode::LowPower:
        return BSEC_SAMPLE_RATE_LP;
    case SensorMode::Continuous:
        return BSEC_SAMPLE_RATE_CONT;
    default:
        /* Let the LowPower config be used as a fallback */
        return BSEC_SAMPLE_RATE_LP;
    }
}

/* Needs to be accessible from the callback function */
static IAQAccuracy s_currentAccuracy = IAQAccuracy::Unreliable;

static bsecSensor s_sensorList[] = {BSEC_OUTPUT_IAQ,
                                    BSEC_OUTPUT_STATIC_IAQ,
                                    BSEC_OUTPUT_CO2_EQUIVALENT,
                                    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
                                    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
                                    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
                                    BSEC_OUTPUT_RAW_TEMPERATURE,
                                    BSEC_OUTPUT_RAW_HUMIDITY,
                                    BSEC_OUTPUT_RAW_PRESSURE,
                                    BSEC_OUTPUT_RAW_GAS};

static const SensorData convertOutputs(const bsecOutputs& p_outputs) {
    SensorData l_data;
    for (uint8_t i = 0; i < p_outputs.nOutputs; i++) {
        const bsecData& o = p_outputs.output[i];

        switch (o.sensor_id) {
        case BSEC_OUTPUT_IAQ:
            l_data.iaq = o.signal;
            l_data.iaqAccuracy = static_cast<IAQAccuracy>(o.accuracy);
            s_currentAccuracy = l_data.iaqAccuracy;
            break;

        case BSEC_OUTPUT_CO2_EQUIVALENT:
            l_data.co2 = o.signal;
            break;

        case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
            l_data.voc = o.signal;
            break;

        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
            l_data.temp = o.signal;
            break;

        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
            l_data.hum = o.signal;
            break;

        case BSEC_OUTPUT_RAW_PRESSURE:
            l_data.pressure = o.signal;
            break;

        case BSEC_OUTPUT_RAW_GAS:
            l_data.gas = o.signal;
            break;

        case BSEC_OUTPUT_RAW_TEMPERATURE:
            l_data.rawTemp = o.signal;
            break;

        case BSEC_OUTPUT_RAW_HUMIDITY:
            l_data.rawHum = o.signal;
            break;

        default:
            break;
        }
    }

    return l_data;
}

void EnvSensor::checkBsecStatus() {
    if (m_bsec.status < BSEC_OK) {
        Serial.printf("BSEC error code: %d\n", m_bsec.status);
    } else if (m_bsec.status > BSEC_OK) {
        Serial.printf("BSEC warning code: %d\n", m_bsec.status);
    }

    if (m_bsec.sensor.status < BME68X_OK) {
        Serial.printf("BME68X error code: %d\n", m_bsec.sensor.status);
    } else if (m_bsec.sensor.status > BME68X_OK) {
        Serial.printf("BME68X warning code: %d\n", m_bsec.sensor.status);
    }
}

bool EnvSensor::init(SensorMode p_mode) {
    m_mode = p_mode;
    s_sensorQueue = xQueueCreate(QUEUE_SIZE, sizeof(SensorData));

    if (!Wire.begin()) {
        Serial.println("Failed to initialize I2C");
        return false;
    }

    if (!m_bsec.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        Serial.println("BME688 init failed");
        checkBsecStatus();
        return false;
    }

    m_bsec.setTemperatureOffset(BME68X_TEMPERATURE_OFFSET);

    if (!m_bsec.setConfig(modeToConfig(m_mode))) {
        Serial.println("BSEC config load failed");
        checkBsecStatus();
        return false;
    }

    if (!m_bsec.updateSubscription(s_sensorList, ARRAY_LEN(s_sensorList), modeToSampleRate(m_mode))) {
        Serial.println("Failed to update subscription");
        checkBsecStatus();
        return false;
    }

    Serial.println("BSEC subscription updated successfully");

    if (auto state = m_storage.loadBsecState(MappingHandler::sensorModeToStorageKey(m_mode))) {
        if (!setBsecState(*state))
            Serial.println("Failed to restore BSEC state");
        else {
            Serial.println("BSEC state restored from storage");
            m_hasSavedStateForMode = true;
        }
    } else {
        Serial.println("No saved BSEC state found");
    }

    m_bsec.attachCallback([](const bme68xData p_data, const bsecOutputs p_outputs, Bsec2 p_bsec) {
        if (!p_outputs.nOutputs) {
            if (p_bsec.status != BSEC_OK)
                Serial.printf("BSEC callback error: %d\n", p_bsec.status);
            return;
        }

        SensorData l_data = convertOutputs(p_outputs);
        xQueueSend(s_sensorQueue, &l_data, TICKS_TO_WAIT);
    });

    Serial.println("==================================");
    Serial.println("BME688 Sensor started");
    if (m_mode == SensorMode::Continuous) {
        Serial.println("Continuous mode (~1s)");
    } else if (m_mode == SensorMode::LowPower) {
        Serial.println("Low Power mode (~3s)");
    } else if (m_mode == SensorMode::UltraLowPower) {
        Serial.println("Ultra Low Power mode (~300s)");
    }
    Serial.println("==================================");

    return true;
}

void EnvSensor::run() {
    if (!m_bsec.run()) {
        Serial.println("BSEC run failed");
        checkBsecStatus();
    }
}

QueueHandle_t EnvSensor::getQueue() const {
    return s_sensorQueue;
}

std::optional<SensorState> EnvSensor::getBsecState() {
    SensorState buf{};
    if (!m_bsec.getState(buf.data())) {
        Serial.printf("Failed to get BSEC state (%d)\n", m_bsec.status);
        return std::nullopt;
    }
    return buf;
}

bool EnvSensor::setBsecState(const SensorState& p_state) {
    if (!m_bsec.setState(const_cast<uint8_t*>(p_state.data()))) {
        Serial.printf("BSEC state failed (%d)\n", m_bsec.status);
        return false;
    }
    return true;
}

bool EnvSensor::setMode(SensorMode p_mode) {

    /* If the  desired mode is already set, no need to do anything */
    if (p_mode == m_mode)
        return true;

    /* Reset the saved state flag */
    m_hasSavedStateForMode = false;

    /* If we're switching modes, we need to set the new config and subscription. BSEC's setConfig() and
       updateSubscription() APIs are designed to be called at runtime, but they reset BSEC's internal state, so we also
       need to persist the learned state for the outgoing mode and restore whatever was previously learned for the new
       one. */
    maybeSaveStateToStorage();

    if (!m_bsec.setConfig(modeToConfig(p_mode))) {
        Serial.println("BSEC config load failed");
        checkBsecStatus();
        return false;
    }

    if (auto state = m_storage.loadBsecState(MappingHandler::sensorModeToStorageKey(p_mode))) {
        if (!this->setBsecState(*state)) {
            Serial.println("Failed to restore BSEC state for new mode");
        } else {
            Serial.println("BSEC state restored from storage for new mode");
            m_hasSavedStateForMode = true;
        }
    } else {
        Serial.println("No saved BSEC state for this mode, starting fresh");
    }

    if (!m_bsec.updateSubscription(s_sensorList, ARRAY_LEN(s_sensorList), modeToSampleRate(p_mode))) {
        Serial.println("Failed to update subscription");
        checkBsecStatus();
        return false;
    }

    Serial.println("BSEC subscription updated successfully");

    m_mode = p_mode;

    Serial.println("==================================");
    Serial.println("BME688 Sensor Mode changed");
    if (m_mode == SensorMode::Continuous) {
        Serial.println("Continuous mode (~1s)");
    } else if (m_mode == SensorMode::LowPower) {
        Serial.println("Low Power mode (~3s)");
    } else if (m_mode == SensorMode::UltraLowPower) {
        Serial.println("Ultra Low Power mode (~300s)");
    }
    Serial.println("==================================");

    return true;
}

void EnvSensor::maybeSaveStateToStorage() {
    bool l_shouldSave = false;

    if (s_currentAccuracy == IAQAccuracy::High) {
        // First mature calibration
        if (!m_hasSavedStateForMode) {
            Serial.println("First High accuracy reached, saving state");
            l_shouldSave = true;
        } else {
            uint64_t nowMs = static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
            if (nowMs - m_lastStateSaveMs >= STATE_SAVE_PERIOD_MS) {
                Serial.println("Periodic BSEC state save triggered");
                l_shouldSave = true;
                m_lastStateSaveMs = nowMs;
            }
        }
    }

    if (l_shouldSave) {
        if (auto state = this->getBsecState()) {
            if (m_storage.saveBsecState(MappingHandler::sensorModeToStorageKey(m_mode), *state)) {
                Serial.println("BSEC state saved");
                m_hasSavedStateForMode = true;
            } else
                Serial.println("BSEC state save failed");
        } else {
            Serial.println("getBsecState() returned nullopt, state not saved");
        }
    }
}

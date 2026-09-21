#include "07_command/command_handler.hpp"

#include "00_vendor/arduino.hpp"

#include <esp_system.h>

constexpr uint32_t QUEUE_LENGTH = 8;
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 1;

CommandHandler::~CommandHandler() {
    if (m_task != nullptr) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
    if (m_queue != nullptr) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }
}

bool CommandHandler::init() {
    m_queue = xQueueCreate(QUEUE_LENGTH, sizeof(Command));
    if (m_queue == nullptr) {
        Serial.println("CommandHandler queue creation failed");
        return false;
    }

    return true;
}

void CommandHandler::start(DisplayController* p_displayController) {
    m_displayController = p_displayController;

    if (pdPASS != xTaskCreate(taskEntry, "command", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task)) {
        Serial.println("CommandHandler task creation failed");
    }
}

void CommandHandler::enqueue(std::string_view p_data) {
    const Command l_cmd = parseCommand(p_data);
    if (xQueueSend(m_queue, &l_cmd, 0) != pdTRUE) {
        Serial.println("[CMD] Command queue full, dropping command");
    }
}

void CommandHandler::taskEntry(void* p_parameter) {
    static_cast<CommandHandler*>(p_parameter)->taskLoop();
}

void CommandHandler::taskLoop() {
    for (;;) {
        Command l_cmd;
        if (xQueueReceive(m_queue, &l_cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        handle(l_cmd);
    }
}

void CommandHandler::handle(Command p_cmd) {
    switch (p_cmd) {
    case Command::DeviceReboot:
        Serial.println("[CMD] Rebooting...");
        m_envSensor.requestModeChange(SensorMode::Disabled);
        m_mqttBridge.disconnect();
        esp_restart();
        break;
    case Command::SensorLowPower:
        Serial.println("[CMD] Switching sensor to Low Power mode");
        m_envSensor.requestModeChange(SensorMode::LowPower);
        break;
    case Command::SensorUltraLowPower:
        Serial.println("[CMD] Switching sensor to Ultra Low Power mode");
        m_envSensor.requestModeChange(SensorMode::UltraLowPower);
        break;
    case Command::DeviceClaimed:
        Serial.println("[CMD] Device claimed");
        m_storage.saveDeviceClaimStatus(true);
        if (m_displayController != nullptr) {
            m_displayController->setClaimedStatus(true);
        }
        break;
    case Command::DeviceUnclaimed:
        Serial.println("[CMD] Device unclaimed");
        m_storage.saveDeviceClaimStatus(false);
        if (m_displayController != nullptr) {
            m_displayController->setClaimedStatus(false);
        }
        Serial.printf("Claim code: %s\n", m_claimCodeManager.get().data());
        break;
    case Command::Unknown:
        Serial.println("[CMD] Unknown command received");
        break;
    }
}

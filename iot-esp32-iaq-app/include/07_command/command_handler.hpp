#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string_view>

#include "00_vendor/freertos.hpp"
#include "01_sensor/env_sensor.hpp"
#include "02_storage/storage.hpp"
#include "04_mqtt/mqtt_bridge.hpp"
#include "06_display/display_controller.hpp"
#include "07_command/command.hpp"
#include "09_utils/claim_code_manager.hpp"

// Executes the commands received over MQTT. The MQTT event callback (called on esp-mqtt's own
// task) only parses and enqueues; the actual handling runs on this class's own task so it
// never blocks the MQTT client task.
class CommandHandler {
public:
    CommandHandler(EnvSensor& p_envSensor, MqttBridge& p_mqttBridge, Storage& p_storage, const ClaimCodeManager& p_claimCodeManager,
                   DisplayController& p_displayController)
        : m_envSensor(p_envSensor), m_mqttBridge(p_mqttBridge), m_storage(p_storage), m_claimCodeManager(p_claimCodeManager),
          m_displayController(p_displayController) {}
    ~CommandHandler();
    CommandHandler(const CommandHandler&) = delete;
    CommandHandler& operator=(const CommandHandler&) = delete;
    CommandHandler(CommandHandler&&) = delete;
    CommandHandler& operator=(CommandHandler&&) = delete;

    // Creates the command queue. Must run before the MQTT client can connect, since a command
    // could otherwise arrive (and be enqueued from the MQTT task) before the queue exists.
    [[nodiscard]] bool init();

    // Starts the task that drains the queue.
    void start();

    // Thread-safe: parses p_data and enqueues the resulting command, dropping it if the queue is full.
    void enqueue(std::string_view p_data);

private:
    static void taskEntry(void* p_parameter);
    void taskLoop();
    void handle(Command p_cmd);

    EnvSensor& m_envSensor;
    MqttBridge& m_mqttBridge;
    Storage& m_storage;
    const ClaimCodeManager& m_claimCodeManager;
    DisplayController& m_displayController;
    QueueHandle_t m_queue = nullptr;
    TaskHandle_t m_task = nullptr;
};

#endif // COMMAND_HANDLER_HPP

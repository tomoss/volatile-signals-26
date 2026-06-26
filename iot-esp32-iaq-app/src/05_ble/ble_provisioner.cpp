#include "05_ble/ble_provisioner.hpp"

#include "00_vendor/arduino.hpp"

#include <algorithm>
#include <esp_random.h>

// Custom 128-bit UUIDs for the WiFi-provisioning GATT service. SERVICE_UUID is a random
// base; each characteristic derives from it by incrementing the leading field, keeping
// the set grouped (...8e9e service, ...8e9f SSID, ...8ea0 password).
constexpr char SERVICE_UUID[] = "fffc8e9e-df11-4c0c-8206-204b2097205c";
constexpr char SSID_CHAR_UUID[] = "fffc8e9f-df11-4c0c-8206-204b2097205c";
constexpr char PASSWORD_CHAR_UUID[] = "fffc8ea0-df11-4c0c-8206-204b2097205c";
constexpr char DEVICE_NAME[] = "ESP32-BLE-SETUP";

// Bluetooth SIG standard 16-bit descriptor UUID 0x2901 (Characteristic User Description):
// attaches a human-readable label to a characteristic so generic scanner apps show e.g.
// "WiFi SSID" instead of the raw UUID. The value is fixed by spec — clients recognize the
// descriptor by this exact number (0x2902 would be the notify-config CCCD, etc.).
constexpr char USER_DESCRIPTION_UUID[] = "2901";

constexpr uint32_t QUEUE_LENGTH = 2;
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr UBaseType_t TASK_PRIORITY = 1;

// Attaches a 0x2901 (Characteristic User Description) descriptor to the given
// characteristic, so scanner apps display p_description as its human-readable label.
static void addUserDescription(NimBLECharacteristic* p_characteristic, const char* p_description) {
    NimBLEDescriptor* l_descriptor = p_characteristic->createDescriptor(USER_DESCRIPTION_UUID, NIMBLE_PROPERTY::READ);
    l_descriptor->setValue(p_description);
}

BleProvisioner::~BleProvisioner() {
    if (m_task != nullptr) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
    if (m_queue != nullptr) {
        vQueueDelete(m_queue);
        m_queue = nullptr;
    }
}

void BleProvisioner::setCredentialsCallback(CredentialsCallback p_callback) {
    m_callback = std::move(p_callback);
}

void BleProvisioner::setPasskeyDisplayCallback(PasskeyDisplayCallback p_callback) {
    m_passkeyDisplayCallback = std::move(p_callback);
}

bool BleProvisioner::init() {
    m_queue = xQueueCreate(QUEUE_LENGTH, sizeof(BleAction));

    if (m_queue == nullptr) {
        return false;
    }

    if (pdPASS != xTaskCreate(taskEntry, "ble", TASK_STACK_SIZE, this, TASK_PRIORITY, &m_task)) {
        return false;
    }

    return true;
}

void BleProvisioner::start() {
    enqueueAction(BleAction::Start);
}

void BleProvisioner::stop() {
    enqueueAction(BleAction::Stop);
}

void BleProvisioner::enqueueAction(BleAction action) {
    if (m_queue == nullptr) {
        return;
    }
    xQueueSend(m_queue, &action, 0);
}

void BleProvisioner::taskEntry(void* parameter) {
    static_cast<BleProvisioner*>(parameter)->taskLoop();
}

void BleProvisioner::taskLoop() {
    for (;;) {
        BleAction l_action;
        if (xQueueReceive(m_queue, &l_action, portMAX_DELAY) == pdTRUE) {
            switch (l_action) {
            case BleAction::Start:
                begin();
                break;
            case BleAction::Stop:
                end();
                break;
            }
        }
    }
}

void BleProvisioner::begin() {
    if (m_running)
        return;

    m_ssid.fill(0);

    NimBLEDevice::init(DEVICE_NAME);
    // Authenticated, MITM-protected pairing using LE Secure Connections. DISPLAY_ONLY makes
    // this device show a passkey that the client must enter, so credential writes only happen
    // on an authenticated link (see the WRITE_AUTHEN characteristics below).
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    m_server = NimBLEDevice::createServer();

    // false: this object outlives the server, so NimBLE must not delete it on teardown.
    m_server->setCallbacks(this, false);

    NimBLEService* l_service = m_server->createService(SERVICE_UUID);

    m_ssidChar = l_service->createCharacteristic(SSID_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_AUTHEN);
    m_ssidChar->setCallbacks(this);
    addUserDescription(m_ssidChar, "WiFi SSID");

    m_passwordChar = l_service->createCharacteristic(PASSWORD_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_AUTHEN);
    m_passwordChar->setCallbacks(this);
    addUserDescription(m_passwordChar, "WiFi Password");

    m_server->start();

    NimBLEAdvertising* l_advertising = NimBLEDevice::getAdvertising();
    l_advertising->setName(DEVICE_NAME);
    l_advertising->addServiceUUID(SERVICE_UUID);
    l_advertising->start();

    m_running = true;
    Serial.println("[BLE] Provisioning started, advertising...");
}

void BleProvisioner::end() {
    if (!m_running)
        return;

    NimBLEDevice::getAdvertising()->stop();

    if (m_server != nullptr && m_server->getConnectedCount() > 0) {
        m_stopPending = true;
        for (uint16_t l_connHandle : m_server->getPeerDevices()) {
            m_server->disconnect(l_connHandle);
        }
        return;
    }

    m_stopPending = false;
    NimBLEDevice::deinit(true);

    m_server = nullptr;
    m_ssidChar = nullptr;
    m_passwordChar = nullptr;
    m_running = false;

    Serial.println("[BLE] Provisioning stopped");
}

void BleProvisioner::onConnect(NimBLEServer* /*p_server*/, NimBLEConnInfo& p_connInfo) {
    Serial.printf("[BLE] Client connected: %s\n", p_connInfo.getAddress().toString().c_str());
}

void BleProvisioner::onDisconnect(NimBLEServer* p_server, NimBLEConnInfo& p_connInfo, int p_reason) {
    Serial.printf("[BLE] Client disconnected: %s (reason=%d)\n", p_connInfo.getAddress().toString().c_str(), p_reason);

    // The disconnect end() requested has now actually completed; re-enqueue Stop so
    // taskLoop() finishes the teardown on our own task instead of from this NimBLE callback.
    if (m_stopPending && p_server->getConnectedCount() == 0) {
        enqueueAction(BleAction::Stop);
    }
}

uint32_t BleProvisioner::onPassKeyDisplay() {
    // Draw the passkey from the hardware RNG so it's unpredictable per pairing. A hardcoded
    // constant could be read straight out of the firmware image, letting an attacker pass the
    // passkey check and defeat the MITM protection.
    uint32_t l_passkey = esp_random() % 1000000;
    if (m_passkeyDisplayCallback) {
        m_passkeyDisplayCallback(l_passkey);
    }
    return l_passkey;
}

void BleProvisioner::onAuthenticationComplete(NimBLEConnInfo& p_connInfo) {
    if (!p_connInfo.isAuthenticated()) {
        Serial.println("[BLE] Pairing failed (not authenticated)");
        return;
    }
    Serial.println("[BLE] Pairing authenticated, credential writes allowed");
}

void BleProvisioner::onWrite(NimBLECharacteristic* p_characteristic, NimBLEConnInfo& /*p_connInfo*/) {
    NimBLEAttValue l_value = p_characteristic->getValue();

    if (p_characteristic == m_ssidChar) {
        if (l_value.size() == 0 || l_value.size() >= m_ssid.size()) {
            Serial.println("[BLE] Invalid SSID, ignoring");
            return;
        }
        std::copy(l_value.begin(), l_value.end(), m_ssid.begin());
        m_ssid[l_value.size()] = '\0';
        Serial.printf("[BLE] SSID received: %s\n", m_ssid.data());
        return;
    }

    if (p_characteristic == m_passwordChar) {
        if (m_ssid[0] == '\0') {
            Serial.println("[BLE] Password received before SSID, ignoring");
            return;
        }
        if (l_value.size() >= m_password.size()) {
            Serial.println("[BLE] Password too long, ignoring");
            return;
        }
        std::copy(l_value.begin(), l_value.end(), m_password.begin());
        m_password[l_value.size()] = '\0';
        Serial.println("[BLE] Password received, invoking callback");
        if (m_callback)
            m_callback(m_ssid, m_password);
    }
}
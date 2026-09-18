#ifndef CLAIM_CODE_MANAGER_HPP
#define CLAIM_CODE_MANAGER_HPP

#include "00_vendor/arduino.hpp"
#include "02_storage/storage.hpp"
#include "07_utils/claim_code.hpp"

#include <cstdio>
#include <esp_random.h>

// Owns the device's claim code: loaded from storage, or generated and persisted as a new
// random one on first boot. Stable for the device's lifetime once generated.
class ClaimCodeManager {
public:
    explicit ClaimCodeManager(Storage& p_storage) : m_storage(p_storage) {}
    ~ClaimCodeManager() = default;
    ClaimCodeManager(const ClaimCodeManager&) = delete;
    ClaimCodeManager& operator=(const ClaimCodeManager&) = delete;
    ClaimCodeManager(ClaimCodeManager&&) = delete;
    ClaimCodeManager& operator=(ClaimCodeManager&&) = delete;

    // Loads the stored code, or generates + persists a new random one if none exists yet.
    // Meant to be called once, in setup().
    void init() {
        if (const auto l_saved = m_storage.loadClaimCode()) {
            m_code = *l_saved;
            return;
        }

        const uint32_t l_random = esp_random() % 1000000;
        snprintf(m_code.data(), m_code.size(), "%06lu", static_cast<unsigned long>(l_random));
        if (!m_storage.saveClaimCode(m_code)) {
            Serial.println("Failed to save claim code");
        }
    }

    const ClaimCode& get() const { return m_code; }

private:
    Storage& m_storage;
    ClaimCode m_code{};
};

#endif // CLAIM_CODE_MANAGER_HPP

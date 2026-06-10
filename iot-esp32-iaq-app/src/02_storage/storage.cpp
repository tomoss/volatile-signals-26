#include "02_storage/storage.h"

std::optional<SensorState> Storage::loadBsecState(StorageKey p_key) {
    SensorState l_state{};
    size_t l_len = get(STORAGE_BSEC_NAMESPACE, p_key, l_state.data(), l_state.size());
    if (l_len != l_state.size())
        return std::nullopt;
    return l_state;
}

bool Storage::saveBsecState(StorageKey p_key, const SensorState& p_state) {
    return put(STORAGE_BSEC_NAMESPACE, p_key, p_state.data(), p_state.size());
}
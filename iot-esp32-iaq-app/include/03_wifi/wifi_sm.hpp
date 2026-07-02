#ifndef WIFI_SM_HPP
#define WIFI_SM_HPP

#include <concepts>
#include <cstdint>

#include "00_vendor/arduino.hpp"
#include "00_vendor/sml.hpp"
#include "wifi_types.hpp"

namespace sml = boost::sml;

// Interface WifiSm's guards/actions need from an adapter — the shape WifiAdapter
// implements for hardware and FakeWifiAdapter implements for tests. Constraining
// TAdapter on this turns a mismatch into a readable "which method is missing/wrong"
// error at the call site, instead of a wall of errors from deep inside boost::sml.
template<typename T>
concept WifiAdaptable = requires(T& adapter) {
    { adapter.loadCredentials() } -> std::convertible_to<bool>;
    { adapter.maxReconnectAttemptsReached() } -> std::convertible_to<bool>;
    { adapter.connect() } -> std::convertible_to<bool>;
    { adapter.getSSID() } -> std::convertible_to<WifiTypes::Ssid>;
    { adapter.getIPAddress() } -> std::convertible_to<WifiTypes::IpAddr>;
    { adapter.resetReconnectAttempts() };
    { adapter.notifyConnected() };
    { adapter.notifyDisconnected() };
    { adapter.recordReconnectAttempt() };
    { adapter.startReconnectTimer() } -> std::convertible_to<bool>;
    { adapter.getReconnectAttempts() } -> std::convertible_to<uint8_t>;
    { adapter.notifyStartProvisioning() };
    { adapter.notifyStopProvisioning() };
};

// Logger for the WiFi state machine
struct WifiSmLogger {
    template<class SM, class TEvent>
    void log_process_event(const TEvent&) {
        // Skip SML's own internal pseudo-events (on_entry/on_exit/anonymous/...) — only
        // print our own events. Their type names are an ugly, deeply-templated mess anyway.
        if constexpr (!sml::aux::is_base_of<sml::back::internal_event, TEvent>::value) {
            Serial.printf("[WiFi SM] Event: %s\n", sml::aux::get_type_name<TEvent>());
        }
    }

    template<class SM, class TGuard, class TEvent>
    void log_guard(const TGuard&, const TEvent&, bool result) {
        Serial.printf("[WiFi SM] Guard: %s: %s\n", sml::aux::get_type_name<TGuard>(), result ? "true" : "false");
    }

    template<class SM, class TAction, class TEvent>
    void log_action(const TAction&, const TEvent&) {}

    template<class SM, class TSrcState, class TDstState>
    void log_state_change(const TSrcState& src, const TDstState& dst) {
        Serial.printf("[WiFi SM] State: %s -> %s\n", src.c_str(), dst.c_str());
    }
};

// ****** EVENTS ******

struct EvReqStart {};
struct EvReqStop {};

struct EvReqConnect {};
struct EvReqDisconnect {};
struct EvReqProvisioning {};
struct EvReqReconnect {};

struct EvIsConnected {};
struct EvIsDisconnected {};
struct EvCredentialsUpdated {};
struct EvReconnectScheduled {};
struct EvReconnectTimeout {};

// ****** STATES ******

struct StIdle {};
struct StDisconnected {};
struct StConnecting {};
struct StConnected {};
struct StReconnectPending {};
struct StProvisioning {};

// ****** GUARDS ******
// Named function objects (not lambdas) so WifiSmLogger::log_guard can print a
// readable type name instead of an anonymous <lambda(...)> signature. Templated on
// WifiAdaptable so the SM can run against a test double without needing the real
// WifiAdaptable (and the Arduino/Preferences dependencies it drags in).

template<WifiAdaptable TAdapter>
struct GuCredentialsLoad {
    bool operator()(TAdapter& p_wifi) const { return p_wifi.loadCredentials(); }
};

template<WifiAdaptable TAdapter>
struct GuMaxAttemptsReached {
    bool operator()(TAdapter& p_wifi) const { return p_wifi.maxReconnectAttemptsReached(); }
};

// ****** ACTIONS ******

template<WifiAdaptable TAdapter>
struct DoAttemptConnect {
    void operator()(TAdapter& p_wifi) const {
        if (!p_wifi.connect()) {
            Serial.println("[WiFi SM] Action: Failed to start WiFi connection attempt!");
            return;
        }
        Serial.printf("Connecting to %s...\n", p_wifi.getSSID().data());
    }
};

template<WifiAdaptable TAdapter>
struct DoNotifyConnect {
    void operator()(TAdapter& p_wifi) const {
        p_wifi.resetReconnectAttempts();
        Serial.printf("[WiFi SM] Action: WiFi connected, IP: %s\n", p_wifi.getIPAddress().data());
        p_wifi.notifyConnected();
    }
};

template<WifiAdaptable TAdapter>
struct DoNotifyDisconnect {
    void operator()(TAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: WiFi disconnected");
        p_wifi.notifyDisconnected();
    }
};

template<WifiAdaptable TAdapter>
struct DoStartTimer {
    void operator()(TAdapter& p_wifi) const {
        p_wifi.recordReconnectAttempt();
        if (p_wifi.startReconnectTimer()) {
            Serial.printf("[WiFi SM] Action: Start timer for reconnecting (Attempt %d)\n", p_wifi.getReconnectAttempts());
        } else {
            Serial.println("[WiFi SM] Action: Failed to schedule reconnect timer!");
        }
    }
};

template<WifiAdaptable TAdapter>
struct DoStartProvisioning {
    void operator()(TAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: Notifying start provisioning");
        p_wifi.resetReconnectAttempts();
        p_wifi.notifyStartProvisioning();
    }
};

template<WifiAdaptable TAdapter>
struct DoStopProvisioning {
    void operator()(TAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: Notifying stop provisioning");
        p_wifi.notifyStopProvisioning();
    }
};

template<WifiAdaptable TAdapter>
struct WifiSm {
    auto operator()() const {
        using namespace sml;

        constexpr auto guCredentialsLoad = GuCredentialsLoad<TAdapter>{};
        constexpr auto guMaxAttemptsReached = GuMaxAttemptsReached<TAdapter>{};

        constexpr auto doAttemptConnect = DoAttemptConnect<TAdapter>{};
        constexpr auto doNotifyConnect = DoNotifyConnect<TAdapter>{};
        constexpr auto doNotifyDisconnect = DoNotifyDisconnect<TAdapter>{};
        constexpr auto doStartTimer = DoStartTimer<TAdapter>{};
        constexpr auto doStartProvisioning = DoStartProvisioning<TAdapter>{};
        constexpr auto doStopProvisioning = DoStopProvisioning<TAdapter>{};

        // TRANSITION TABLE: src_state + event [ guard ] / action = dst_state
        return make_transition_table(
            // Initial state -> StIdle
            *state<StIdle> + event<EvReqStart>[guCredentialsLoad] / doAttemptConnect = state<StConnecting>,
            state<StIdle> + event<EvReqStart>[!guCredentialsLoad] / doStartProvisioning = state<StProvisioning>,

            state<StConnecting> + event<EvIsConnected> / doNotifyConnect = state<StConnected>,
            state<StConnecting> + event<EvIsDisconnected> / doNotifyDisconnect = state<StDisconnected>,

            state<StConnected> + event<EvIsDisconnected> / doNotifyDisconnect = state<StDisconnected>,

            state<StDisconnected> + event<EvReqReconnect>[!guMaxAttemptsReached] / doStartTimer = state<StReconnectPending>,
            state<StDisconnected> + event<EvReqReconnect>[guMaxAttemptsReached] / doStartProvisioning = state<StProvisioning>,

            // Reconnect timer fired; actually attempt the connection now.
            state<StReconnectPending> + event<EvReqConnect> / doAttemptConnect = state<StConnecting>,

            state<StProvisioning> + event<EvCredentialsUpdated>[guCredentialsLoad] / (doStopProvisioning, doAttemptConnect) = state<StConnecting>,
            state<StProvisioning> + event<EvCredentialsUpdated>[!guCredentialsLoad] = state<StProvisioning>);
    }
};

#endif // WIFI_SM_HPP
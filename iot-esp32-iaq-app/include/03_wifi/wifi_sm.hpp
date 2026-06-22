#ifndef WIFI_SM_HPP
#define WIFI_SM_HPP

#include "00_vendor/sml.hpp"
#include "02_storage/storage.hpp"
#include "wifi_adapter.hpp"

namespace sml = boost::sml;

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
// readable type name instead of an anonymous <lambda(...)> signature.
struct GuCredentialsLoad {
    bool operator()(WifiAdapter& p_wifi) const { return p_wifi.loadCredentials(); }
};

struct GuMaxAttemptsReached {
    bool operator()(WifiAdapter& p_wifi) const { return p_wifi.maxReconnectAttemptsReached(); }
};

// ****** ACTIONS ******
struct DoAttemptConnect {
    void operator()(WifiAdapter& p_wifi) const {
        if (!p_wifi.connect()) {
            Serial.println("[WiFi SM] Action: Failed to start WiFi connection attempt!");
            return;
        }
        Serial.printf("Connecting to %s...\n", p_wifi.getSSID().data());
    }
};

struct DoNotifyConnect {
    void operator()(WifiAdapter& p_wifi) const {
        Serial.printf("[WiFi SM] Action: WiFi connected, IP: %s\n", p_wifi.getIPAddress().data());
        p_wifi.notifyConnected();
    }
};

struct DoNotifyDisconnect {
    void operator()(WifiAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: WiFi disconnected");
        p_wifi.notifyDisconnected();
    }
};

struct DoStartTimer {
    void operator()(WifiAdapter& p_wifi) const {
        p_wifi.recordReconnectAttempt();
        if (p_wifi.startReconnectTimer()) {
            Serial.printf("[WiFi SM] Action: Start timer for reconnecting (Attempt %d)\n", p_wifi.getReconnectAttempts());
        } else {
            Serial.println("[WiFi SM] Action: Failed to schedule reconnect timer!");
        }
    }
};

struct DoStartProvisioning {
    void operator()(WifiAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: Notifying start provisioning");
        p_wifi.resetReconnectAttempts();
        p_wifi.notifyStartProvisioning();
    }
};

struct DoStopProvisioning {
    void operator()(WifiAdapter& p_wifi) const {
        Serial.println("[WiFi SM] Action: Notifying stop provisioning");
        p_wifi.notifyStopProvisioning();
    }
};

struct WifiSm {
    auto operator()() const {
        using namespace sml;

        constexpr auto guCredentialsLoad = GuCredentialsLoad{};
        constexpr auto guMaxAttemptsReached = GuMaxAttemptsReached{};

        constexpr auto doAttemptConnect = DoAttemptConnect{};
        constexpr auto doNotifyConnect = DoNotifyConnect{};
        constexpr auto doNotifyDisconnect = DoNotifyDisconnect{};
        constexpr auto doStartTimer = DoStartTimer{};
        constexpr auto doStartProvisioning = DoStartProvisioning{};
        constexpr auto doStopProvisioning = DoStopProvisioning{};

        // TRANSITION TABLE: src_state + event [ guard ] / action = dst_state
        return make_transition_table(
            // Initial state -> StIdle
            *state<StIdle> + event<EvReqStart>[guCredentialsLoad] / doAttemptConnect = state<StConnecting>,
            state<StIdle> + event<EvReqStart>[!guCredentialsLoad] = state<StProvisioning>,

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
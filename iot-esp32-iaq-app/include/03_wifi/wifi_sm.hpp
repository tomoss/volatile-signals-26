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
        Serial.printf("[WiFi SM] event %s\n", sml::aux::get_type_name<TEvent>());
    }

    template<class SM, class TGuard, class TEvent>
    void log_guard(const TGuard&, const TEvent&, bool result) {
        Serial.printf("[WiFi SM] guard %s: %s\n", sml::aux::get_type_name<TGuard>(), result ? "true" : "false");
    }

    template<class SM, class TAction, class TEvent>
    void log_action(const TAction&, const TEvent&) {}

    template<class SM, class TSrcState, class TDstState>
    void log_state_change(const TSrcState& src, const TDstState& dst) {
        Serial.printf("[WiFi SM] %s -> %s\n", src.c_str(), dst.c_str());
    }
};

// ****** EVENTS ******
struct EvStart {};

struct EvRequestConnect {};
struct EvRequestReconnect {};

struct EvIsConnected {};
struct EvIsDisconnected {};

struct EvRequestProvisioning {};
struct EvCredentialsUpdated {};

// ****** STATES ******
struct StUninitialized {};
struct StDisconnected {};
struct StConnecting {};
struct StConnected {};
struct StProvisioning {};

// ****** GUARDS ******
// Named function objects (not lambdas) so WifiSmLogger::log_guard can print a
// readable type name instead of an anonymous <lambda(...)> signature.
struct GuStartConnect {
    bool operator()(WifiAdapter& p_wifi) const {
        Serial.println("TryConnect guard");
        return p_wifi.connect();
    }
};

struct GuStartReconnect {
    bool operator()(WifiAdapter& p_wifi) const {
        Serial.println("TryReconnect guard");
        return p_wifi.reconnect();
    }
};

struct GuCredentialsExist {
    bool operator()(WifiAdapter& p_wifi, Storage& p_storage) const {
        auto l_ssid = p_storage.loadWifiSSID();
        auto l_pass = p_storage.loadWifiPass();
        if (!l_ssid || !l_pass) {
            Serial.println("No WiFi credentials in storage");
            return false;
        }
        Serial.println("Loaded Wifi credentials from storage");
        p_wifi.setCredentials(*l_ssid, *l_pass);
        return p_wifi.connect();
    }
};

// ****** ACTIONS ******
struct DoConnectStatus {
    void operator()(WifiAdapter& p_wifi) const {
        Serial.printf("[action] WiFi connected, IP: %s\n", p_wifi.getIPAddress().data());
    }
};

struct DoDisconnectStatus {
    void operator()() const { Serial.println("[action] WiFi disconnected"); }
};

struct WifiSm {
    auto operator()() const {
        using namespace sml;

        constexpr auto guStartConnect = GuStartConnect{};
        constexpr auto guStartReconnect = GuStartReconnect{};
        constexpr auto guCredentialsExist = GuCredentialsExist{};

        constexpr auto doConnectStatus = DoConnectStatus{};
        constexpr auto doDisconnectStatus = DoDisconnectStatus{};

        // TRANSITION TABLE: src_state + event [ guard ] / action = dst_state
        return make_transition_table(
            // Initial state -> StUninitialized

            // Loading credentials to go from StUninitialized to StConnecting or StProvisioning
            *state<StUninitialized> + event<EvStart>[guCredentialsExist] = state<StConnecting>,
            state<StUninitialized> + event<EvStart>[!guCredentialsExist] = state<StProvisioning>,

            // After successfully getting the new credentials from provisioning, go to StConnecting
            state<StProvisioning> + event<EvCredentialsUpdated>[guCredentialsExist] = state<StConnecting>,
            state<StProvisioning> + event<EvCredentialsUpdated>[!guCredentialsExist] = state<StProvisioning>,

            // Go to provisioning after we cannot connect with stored credentials,
            // or if the user explicitly requests provisioning while disconnected
            state<StDisconnected> + event<EvRequestProvisioning> = state<StProvisioning>,

            // Connect and reconnect requests while disconnected
            state<StDisconnected> + event<EvRequestConnect>[guStartConnect] = state<StConnecting>,
            state<StDisconnected> + event<EvRequestReconnect>[guStartReconnect] = state<StConnecting>,

            // WiFi.begin() fires a spurious DISCONNECTED before connecting, which can
            // kick the SM back to Disconnected. Accept GOT_IP from either waiting state.
            state<StDisconnected> + event<EvIsConnected> / doConnectStatus = state<StConnected>,

            // If WiFi callback reports multiple disconnects in a row, just update the status without trying to
            // disconnect again
            state<StDisconnected> + event<EvIsDisconnected> / doDisconnectStatus = state<StDisconnected>,

            // If WiFi callback reports we're connected or disconnected, update our state and status accordingly
            state<StConnecting> + event<EvIsConnected> / doConnectStatus = state<StConnected>,
            state<StConnecting> + event<EvIsDisconnected> / doDisconnectStatus = state<StDisconnected>,

            // state<StConnecting> + event<EvRequestReconnect> = state<StConnecting>,

            // state<StConnected> + event<EvRequestReconnect> = state<StConnected>);

            // If we get disconnected while connected, update status and go to Disconnected.
            state<StConnected> + event<EvIsDisconnected> / doDisconnectStatus = state<StDisconnected>);
    }
};

#endif // WIFI_SM_HPP
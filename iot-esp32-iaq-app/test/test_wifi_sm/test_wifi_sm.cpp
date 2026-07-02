#include <gtest/gtest.h>

#include "03_wifi/wifi_sm.hpp"
#include "03_wifi/wifi_types.hpp"

// Plain test double - no inheritance, no vendor types. WifiSm<TAdapter> is templated on
// the adapter type precisely so this can stand in for the real WifiAdapter (see
// wifi_sm.hpp) without needing Arduino/WiFi/FreeRTOS at all.
struct FakeWifiAdapter {
    bool credentialsAvailable = true;
    bool maxAttemptsReached = false;
    bool connectSucceeds = true;
    bool reconnectTimerSucceeds = true;
    uint8_t reconnectAttempts = 0;

    int connectCallCount = 0;
    int notifyConnectedCallCount = 0;
    int notifyDisconnectedCallCount = 0;
    int resetReconnectAttemptsCallCount = 0;
    int recordReconnectAttemptCallCount = 0;
    int startReconnectTimerCallCount = 0;
    int notifyStartProvisioningCallCount = 0;
    int notifyStopProvisioningCallCount = 0;

    bool loadCredentials() { return credentialsAvailable; }
    bool maxReconnectAttemptsReached() const { return maxAttemptsReached; }
    bool connect() {
        ++connectCallCount;
        return connectSucceeds;
    }
    WifiTypes::Ssid getSSID() const { return {}; }
    WifiTypes::IpAddr getIPAddress() const { return {}; }
    void resetReconnectAttempts() { ++resetReconnectAttemptsCallCount; }
    void notifyConnected() { ++notifyConnectedCallCount; }
    void notifyDisconnected() { ++notifyDisconnectedCallCount; }
    void recordReconnectAttempt() { ++recordReconnectAttemptCallCount; }
    bool startReconnectTimer() {
        ++startReconnectTimerCallCount;
        return reconnectTimerSucceeds;
    }
    uint8_t getReconnectAttempts() const { return reconnectAttempts; }
    void notifyStartProvisioning() { ++notifyStartProvisioningCallCount; }
    void notifyStopProvisioning() { ++notifyStopProvisioningCallCount; }
};

using TestSm = sml::sm<WifiSm<FakeWifiAdapter>>;

// gtest builds a fresh fixture instance per TEST_F, so adapter/sm need no manual
// reset/teardown - declaration order (adapter before sm) guarantees adapter is
// constructed before sm's default member initializer binds a reference to it.
class WifiSmTest : public ::testing::Test {
protected:
    FakeWifiAdapter adapter;
    TestSm sm{adapter};
};

TEST_F(WifiSmTest, IdleStartWithCredentialsConnects) {
    adapter.credentialsAvailable = true;

    sm.process_event(EvReqStart{});

    EXPECT_TRUE(sm.is(sml::state<StConnecting>));
    EXPECT_EQ(1, adapter.connectCallCount);
}

TEST_F(WifiSmTest, IdleStartWithoutCredentialsProvisions) {
    adapter.credentialsAvailable = false;

    sm.process_event(EvReqStart{});

    EXPECT_TRUE(sm.is(sml::state<StProvisioning>));
    EXPECT_EQ(0, adapter.connectCallCount);
    // Regression check: this is the cold-boot path (StIdle straight into StProvisioning),
    // distinct from the StDisconnected -> StProvisioning path after max reconnect attempts.
    // Both must actually notify - it's easy to wire the action onto only one of the two
    // transitions in the table and have this one silently do nothing.
    EXPECT_EQ(1, adapter.notifyStartProvisioningCallCount);
}

TEST_F(WifiSmTest, ConnectingIsConnectedNotifies) {
    adapter.credentialsAvailable = true;
    sm.process_event(EvReqStart{}); // -> StConnecting

    sm.process_event(EvIsConnected{});

    EXPECT_TRUE(sm.is(sml::state<StConnected>));
    EXPECT_EQ(1, adapter.notifyConnectedCallCount);
    EXPECT_EQ(1, adapter.resetReconnectAttemptsCallCount);
}

TEST_F(WifiSmTest, ConnectingIsDisconnectedNotifies) {
    adapter.credentialsAvailable = true;
    sm.process_event(EvReqStart{}); // -> StConnecting

    sm.process_event(EvIsDisconnected{});

    EXPECT_TRUE(sm.is(sml::state<StDisconnected>));
    EXPECT_EQ(1, adapter.notifyDisconnectedCallCount);
}

TEST_F(WifiSmTest, ConnectedIsDisconnectedNotifies) {
    adapter.credentialsAvailable = true;
    sm.process_event(EvReqStart{});    // -> StConnecting
    sm.process_event(EvIsConnected{}); // -> StConnected

    sm.process_event(EvIsDisconnected{});

    EXPECT_TRUE(sm.is(sml::state<StDisconnected>));
    EXPECT_EQ(1, adapter.notifyDisconnectedCallCount);
}

TEST_F(WifiSmTest, DisconnectedReconnectUnderMaxStartsTimer) {
    adapter.credentialsAvailable = true;
    adapter.maxAttemptsReached = false;
    sm.process_event(EvReqStart{});       // -> StConnecting
    sm.process_event(EvIsDisconnected{}); // -> StDisconnected

    sm.process_event(EvReqReconnect{});

    EXPECT_TRUE(sm.is(sml::state<StReconnectPending>));
    EXPECT_EQ(1, adapter.recordReconnectAttemptCallCount);
    EXPECT_EQ(1, adapter.startReconnectTimerCallCount);
}

TEST_F(WifiSmTest, DisconnectedReconnectAtMaxProvisions) {
    adapter.credentialsAvailable = true;
    adapter.maxAttemptsReached = true;
    sm.process_event(EvReqStart{});       // -> StConnecting
    sm.process_event(EvIsDisconnected{}); // -> StDisconnected

    sm.process_event(EvReqReconnect{});

    EXPECT_TRUE(sm.is(sml::state<StProvisioning>));
    EXPECT_EQ(1, adapter.notifyStartProvisioningCallCount);
    EXPECT_EQ(1, adapter.resetReconnectAttemptsCallCount);
}

TEST_F(WifiSmTest, ReconnectPendingReqConnectReattempts) {
    adapter.credentialsAvailable = true;
    adapter.maxAttemptsReached = false;
    sm.process_event(EvReqStart{});       // -> StConnecting, connect() #1
    sm.process_event(EvIsDisconnected{}); // -> StDisconnected
    sm.process_event(EvReqReconnect{});   // -> StReconnectPending

    sm.process_event(EvReqConnect{});

    EXPECT_TRUE(sm.is(sml::state<StConnecting>));
    EXPECT_EQ(2, adapter.connectCallCount);
}

TEST_F(WifiSmTest, ProvisioningCredentialsUpdatedWithValidCredsConnects) {
    adapter.credentialsAvailable = false;
    sm.process_event(EvReqStart{}); // -> StProvisioning

    adapter.credentialsAvailable = true;
    sm.process_event(EvCredentialsUpdated{});

    EXPECT_TRUE(sm.is(sml::state<StConnecting>));
    EXPECT_EQ(1, adapter.notifyStopProvisioningCallCount);
    EXPECT_EQ(1, adapter.connectCallCount);
}

TEST_F(WifiSmTest, ProvisioningCredentialsUpdatedWithoutValidCredsStays) {
    adapter.credentialsAvailable = false;
    sm.process_event(EvReqStart{}); // -> StProvisioning

    sm.process_event(EvCredentialsUpdated{});

    EXPECT_TRUE(sm.is(sml::state<StProvisioning>));
    EXPECT_EQ(0, adapter.connectCallCount);
    EXPECT_EQ(0, adapter.notifyStopProvisioningCallCount);
}

// Informational: EvReqStop/EvReqDisconnect/EvReqProvisioning/EvReconnectScheduled/
// EvReconnectTimeout are declared in wifi_sm.hpp but never appear in
// make_transition_table(...). This test documents that, today, posting one of them is a
// silent no-op rather than wired-up behavior.
TEST_F(WifiSmTest, UnhandledEventIsNoop) {
    adapter.credentialsAvailable = true;
    sm.process_event(EvReqStart{});    // -> StConnecting
    sm.process_event(EvIsConnected{}); // -> StConnected

    const bool l_handled = sm.process_event(EvReqStop{});

    EXPECT_FALSE(l_handled);
    EXPECT_TRUE(sm.is(sml::state<StConnected>));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

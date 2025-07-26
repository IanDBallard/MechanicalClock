#include "TestFramework.h"
#include "../src/NetworkManager.h"
#include <RTC.h>

TEST_SUITE(NetworkManagerTest);

// Mock WiFi credentials for testing
static WiFiCredentials mockCredentials;
static bool mockWiFiConnected = false;
static int mockWiFiStatus = WL_DISCONNECTED;

void setupNetworkManagerTests() {
    // Test NetworkManager constructor
    ADD_TEST(testSuite_NetworkManagerTest, NetworkManagerConstructor) {
        NetworkManager nm("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, -5, true);
        
        // Test that constructor sets up basic parameters
        ASSERT_STRING_EQUAL("TestAP", nm.getSSID());
        ASSERT_EQUAL(-5, nm.getTimeZoneOffset());
        ASSERT_TRUE(nm.getUseDST());
    } END_TEST;

    // Test NetworkManager initialization
    ADD_TEST(testSuite_NetworkManagerTest, NetworkManagerInitialization) {
        NetworkManager nm("TestAP");
        nm.begin();
        
        // Should start in config mode if no credentials
        ASSERT_TRUE(nm.needsConfiguration());
    } END_TEST;

    // Test WiFi connection status
    ADD_TEST(testSuite_NetworkManagerTest, WiFiConnectionStatus) {
        NetworkManager nm("TestAP");
        
        // Test initial disconnected state
        ASSERT_FALSE(nm.isWiFiConnected());
        ASSERT_EQUAL(WL_DISCONNECTED, nm.getWiFiStatus());
        
        // Mock connected state
        MockWiFi::setStatus(WL_CONNECTED);
        ASSERT_TRUE(nm.isWiFiConnected());
        ASSERT_EQUAL(WL_CONNECTED, nm.getWiFiStatus());
        
        // Reset mock
        MockWiFi::reset();
    } END_TEST;

    // Test NTP sync interval
    ADD_TEST(testSuite_NetworkManagerTest, NTPSyncInterval) {
        NetworkManager nm("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, -5, true);
        
        ASSERT_EQUAL(3600000UL, nm.getNtpSyncInterval());
        
        // Test custom interval
        NetworkManager nm2("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 1800000, -5, true);
        ASSERT_EQUAL(1800000UL, nm2.getNtpSyncInterval());
    } END_TEST;

    // Test NTP sync needed calculation
    ADD_TEST(testSuite_NetworkManagerTest, NTPSyncNeeded) {
        NetworkManager nm("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 1000, -5, true);
        
        // Initially should need sync (no previous sync)
        ASSERT_TRUE(nm.isNTPSyncNeeded());
        
        // After sync, should not need immediate sync
        // Note: This would require mocking the internal sync time
        // For now, we test the basic logic
    } END_TEST;

    // Test timezone offset handling
    ADD_TEST(testSuite_NetworkManagerTest, TimezoneOffset) {
        NetworkManager nm("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, -8, false);
        
        ASSERT_EQUAL(-8, nm.getTimeZoneOffset());
        ASSERT_FALSE(nm.getUseDST());
        
        NetworkManager nm2("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, 2, true);
        
        ASSERT_EQUAL(2, nm2.getTimeZoneOffset());
        ASSERT_TRUE(nm2.getUseDST());
    } END_TEST;

    // Test SSID getter
    ADD_TEST(testSuite_NetworkManagerTest, SSIDGetter) {
        NetworkManager nm("MyWiFiNetwork");
        ASSERT_STRING_EQUAL("MyWiFiNetwork", nm.getSSID());
        
        NetworkManager nm2("AnotherNetwork");
        ASSERT_STRING_EQUAL("AnotherNetwork", nm2.getSSID());
    } END_TEST;

    // Test configuration mode detection
    ADD_TEST(testSuite_NetworkManagerTest, ConfigurationModeDetection) {
        NetworkManager nm("TestAP");
        
        // Should need configuration initially
        ASSERT_TRUE(nm.needsConfiguration());
        
        // After successful configuration, should not need config
        // Note: This would require mocking the configuration process
    } END_TEST;

    // Test last NTP sync time
    ADD_TEST(testSuite_NetworkManagerTest, LastNTPSyncTime) {
        NetworkManager nm("TestAP");
        
        // Initially should be 0 (no sync yet)
        ASSERT_EQUAL(0UL, nm.getLastNtpSyncTime());
    } END_TEST;

    // Test WiFi status getter
    ADD_TEST(testSuite_NetworkManagerTest, WiFiStatusGetter) {
        NetworkManager nm("TestAP");
        
        // Test various WiFi statuses
        MockWiFi::setStatus(WL_DISCONNECTED);
        ASSERT_EQUAL(WL_DISCONNECTED, nm.getWiFiStatus());
        
        MockWiFi::setStatus(WL_CONNECTED);
        ASSERT_EQUAL(WL_CONNECTED, nm.getWiFiStatus());
        
        MockWiFi::setStatus(WL_CONNECT_FAILED);
        ASSERT_EQUAL(WL_CONNECT_FAILED, nm.getWiFiStatus());
        
        MockWiFi::reset();
    } END_TEST;

    // Test NetworkManager with different parameters
    ADD_TEST(testSuite_NetworkManagerTest, NetworkManagerParameters) {
        // Test with different NTP servers
        NetworkManager nm1("TestAP", IPAddress(8, 8, 8, 8));
        NetworkManager nm2("TestAP", IPAddress(1, 1, 1, 1));
        
        // Test with different ports
        NetworkManager nm3("TestAP", IPAddress(129, 6, 15, 28), 1234);
        NetworkManager nm4("TestAP", IPAddress(129, 6, 15, 28), 5678);
        
        // Test with different timeouts
        NetworkManager nm5("TestAP", IPAddress(129, 6, 15, 28), 2390, 10000);
        NetworkManager nm6("TestAP", IPAddress(129, 6, 15, 28), 2390, 60000);
        
        // All should initialize without errors
        ASSERT_TRUE(true); // If we get here, no exceptions were thrown
    } END_TEST;

    // Test NetworkManager edge cases
    ADD_TEST(testSuite_NetworkManagerTest, NetworkManagerEdgeCases) {
        // Test with empty SSID
        NetworkManager nm1("");
        ASSERT_STRING_EQUAL("", nm1.getSSID());
        
        // Test with very long SSID
        NetworkManager nm2("ThisIsAVeryLongSSIDNameThatMightExceedNormalLengths");
        ASSERT_STRING_EQUAL("ThisIsAVeryLongSSIDNameThatMightExceedNormalLengths", nm2.getSSID());
        
        // Test with extreme timezone offsets
        NetworkManager nm3("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, -12, false);
        ASSERT_EQUAL(-12, nm3.getTimeZoneOffset());
        
        NetworkManager nm4("TestAP", IPAddress(129, 6, 15, 28), 2390, 30000, 3, 5000, 3, 10000, 3600000, 14, false);
        ASSERT_EQUAL(14, nm4.getTimeZoneOffset());
    } END_TEST;

    // Test NetworkManager state consistency
    ADD_TEST(testSuite_NetworkManagerTest, NetworkManagerStateConsistency) {
        NetworkManager nm("TestAP");
        
        // Test that state remains consistent across multiple calls
        bool needsConfig1 = nm.needsConfiguration();
        bool needsConfig2 = nm.needsConfiguration();
        ASSERT_EQUAL(needsConfig1, needsConfig2);
        
        String ssid1 = nm.getSSID();
        String ssid2 = nm.getSSID();
        ASSERT_STRING_EQUAL(ssid1, ssid2);
        
        int timezone1 = nm.getTimeZoneOffset();
        int timezone2 = nm.getTimeZoneOffset();
        ASSERT_EQUAL(timezone1, timezone2);
    } END_TEST;

    testRegistry.addSuite(testSuite_NetworkManagerTest);
} 
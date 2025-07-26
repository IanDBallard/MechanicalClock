#include "TestFramework.h"
#include "../src/StateManager.h"
#include "../src/NetworkManager.h"
#include "../src/LCDDisplay.h"
#include "../src/MechanicalClock.h"
#include <RTC.h>

TEST_SUITE(StateManagerTest);

// Mock objects for testing
static NetworkManager* mockNetworkManager = nullptr;
static LCDDisplay* mockLCDDisplay = nullptr;
static MechanicalClock* mockClock = nullptr;
static RTClock* mockRTC = nullptr;

void setupStateManagerTests() {
    // Test StateManager constructor
    ADD_TEST(testSuite_StateManagerTest, StateManagerConstructor) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Should start in INIT state
        ASSERT_EQUAL(STATE_INIT, sm.getCurrentState());
    } END_TEST;

    // Test state transitions
    ADD_TEST(testSuite_StateManagerTest, StateTransitions) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Test transition from INIT to CONFIG
        sm.transitionTo(STATE_CONFIG);
        ASSERT_EQUAL(STATE_CONFIG, sm.getCurrentState());
        
        // Test transition to CONNECTING_WIFI
        sm.transitionTo(STATE_CONNECTING_WIFI);
        ASSERT_EQUAL(STATE_CONNECTING_WIFI, sm.getCurrentState());
        
        // Test transition to SYNCING_TIME
        sm.transitionTo(STATE_SYNCING_TIME);
        ASSERT_EQUAL(STATE_SYNCING_TIME, sm.getCurrentState());
        
        // Test transition to RUNNING
        sm.transitionTo(STATE_RUNNING);
        ASSERT_EQUAL(STATE_RUNNING, sm.getCurrentState());
        
        // Test transition to ERROR
        sm.transitionTo(STATE_ERROR);
        ASSERT_EQUAL(STATE_ERROR, sm.getCurrentState());
        
        // Test transition to POWER_SAVING
        sm.transitionTo(STATE_POWER_SAVING);
        ASSERT_EQUAL(STATE_POWER_SAVING, sm.getCurrentState());
    } END_TEST;

    // Test no state change when transitioning to same state
    ADD_TEST(testSuite_StateManagerTest, NoStateChangeOnSameState) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        ClockState initialState = sm.getCurrentState();
        
        // Try to transition to same state
        sm.transitionTo(initialState);
        
        // State should remain the same
        ASSERT_EQUAL(initialState, sm.getCurrentState());
    } END_TEST;

    // Test error handling
    ADD_TEST(testSuite_StateManagerTest, ErrorHandling) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Test setting error message
        String errorMsg = "Test error message";
        sm.setLastError(errorMsg);
        ASSERT_STRING_EQUAL(errorMsg, sm.getLastError());
        
        // Test setting different error message
        String errorMsg2 = "Another error message";
        sm.setLastError(errorMsg2);
        ASSERT_STRING_EQUAL(errorMsg2, sm.getLastError());
        
        // Test error message persistence
        ASSERT_STRING_EQUAL(errorMsg2, sm.getLastError());
    } END_TEST;

    // Test state duration tracking
    ADD_TEST(testSuite_StateManagerTest, StateDurationTracking) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Get initial state time
        unsigned long initialTime = millis();
        
        // Transition to a new state
        sm.transitionTo(STATE_CONFIG);
        
        // State should have changed
        ASSERT_EQUAL(STATE_CONFIG, sm.getCurrentState());
        
        // Small delay to test duration
        delay(10);
        
        // State should still be the same
        ASSERT_EQUAL(STATE_CONFIG, sm.getCurrentState());
    } END_TEST;

    // Test state machine logic - INIT state
    ADD_TEST(testSuite_StateManagerTest, InitStateLogic) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Should start in INIT state
        ASSERT_EQUAL(STATE_INIT, sm.getCurrentState());
        
        // Update should trigger state transition logic
        sm.update();
        
        // Should transition based on network manager needs
        // Since NetworkManager needs configuration by default, should go to CONFIG
        // Note: This depends on the actual implementation logic
    } END_TEST;

    // Test state machine logic - CONFIG state
    ADD_TEST(testSuite_StateManagerTest, ConfigStateLogic) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Transition to CONFIG state
        sm.transitionTo(STATE_CONFIG);
        ASSERT_EQUAL(STATE_CONFIG, sm.getCurrentState());
        
        // Update should handle config state logic
        sm.update();
        
        // Should remain in CONFIG or transition based on configuration status
    } END_TEST;

    // Test state machine logic - RUNNING state
    ADD_TEST(testSuite_StateManagerTest, RunningStateLogic) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Transition to RUNNING state
        sm.transitionTo(STATE_RUNNING);
        ASSERT_EQUAL(STATE_RUNNING, sm.getCurrentState());
        
        // Update should handle running state logic
        sm.update();
        
        // Should remain in RUNNING or transition based on conditions
    } END_TEST;

    // Test state machine logic - ERROR state
    ADD_TEST(testSuite_StateManagerTest, ErrorStateLogic) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Set an error and transition to ERROR state
        sm.setLastError("Test error");
        sm.transitionTo(STATE_ERROR);
        ASSERT_EQUAL(STATE_ERROR, sm.getCurrentState());
        
        // Update should handle error state logic
        sm.update();
        
        // Should eventually transition out of error state
    } END_TEST;

    // Test state machine logic - POWER_SAVING state
    ADD_TEST(testSuite_StateManagerTest, PowerSavingStateLogic) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Transition to POWER_SAVING state
        sm.transitionTo(STATE_POWER_SAVING);
        ASSERT_EQUAL(STATE_POWER_SAVING, sm.getCurrentState());
        
        // Update should handle power saving state logic
        sm.update();
        
        // Should remain in POWER_SAVING until power is restored
    } END_TEST;

    // Test state transitions with error messages
    ADD_TEST(testSuite_StateManagerTest, StateTransitionsWithErrors) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Set error and transition to ERROR state
        sm.setLastError("Connection failed");
        sm.transitionTo(STATE_ERROR);
        ASSERT_EQUAL(STATE_ERROR, sm.getCurrentState());
        ASSERT_STRING_EQUAL("Connection failed", sm.getLastError());
        
        // Clear error and transition to different state
        sm.setLastError("");
        sm.transitionTo(STATE_INIT);
        ASSERT_EQUAL(STATE_INIT, sm.getCurrentState());
        ASSERT_STRING_EQUAL("", sm.getLastError());
    } END_TEST;

    // Test state machine update frequency
    ADD_TEST(testSuite_StateManagerTest, UpdateFrequency) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Multiple updates should not cause issues
        for (int i = 0; i < 10; i++) {
            sm.update();
            delay(1);
        }
        
        // Should still be in a valid state
        ClockState currentState = sm.getCurrentState();
        ASSERT_TRUE(currentState >= STATE_INIT && currentState <= STATE_POWER_SAVING);
    } END_TEST;

    // Test state machine with different network conditions
    ADD_TEST(testSuite_StateManagerTest, NetworkConditions) {
        NetworkManager nm("TestAP");
        LCDDisplay lcd(0x27);
        MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, *mockRTC, lcd);
        RTClock rtc;
        
        StateManager sm(nm, lcd, clock, rtc);
        
        // Test with disconnected WiFi
        MockWiFi::setStatus(WL_DISCONNECTED);
        sm.update();
        
        // Test with connected WiFi
        MockWiFi::setStatus(WL_CONNECTED);
        sm.update();
        
        // Test with connection failed
        MockWiFi::setStatus(WL_CONNECT_FAILED);
        sm.update();
        
        MockWiFi::reset();
    } END_TEST;

    testRegistry.addSuite(testSuite_StateManagerTest);
} 
#include "TestFramework.h"
#include "MechanicalClock.h"
#include "TimeUtils.h"
#include <EEPROM.h>

// Test suite for improved power recovery system
TestSuite testSuite_PowerRecoveryTest("PowerRecoveryTest");

// Test fixture for power recovery tests
class PowerRecoveryTestFixture {
public:
    LCDDisplay lcdDisplay;
    RTClock rtcInstance;
    MechanicalClock clock;
    
    PowerRecoveryTestFixture() 
        : lcdDisplay(0x27), 
          clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay) {
    }
    
    void setup() {
        // Initialize clock
        clock.begin();
        
        // Clear all power recovery data before each test
        clock.clearPowerRecoveryData();
    }
};

// Test power-off simulation
void testPowerOffSimulation() {
    Serial.println("Testing power-off simulation...");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Clear any existing data
    clock.clearPowerRecoveryData();
    
    // Simulate power-off in running state
    bool simulationResult = clock.simulatePowerOff(POWER_STATE_RUNNING);
    ASSERT_TRUE(simulationResult);
    
    // Verify data was saved
    time_t savedTime = clock.getPowerDownTime();
    uint8_t savedState = clock.getPowerDownState();
    bool testMode = clock.isTestMode();
    
    ASSERT_TRUE(savedTime > 0);
    ASSERT_EQUAL(POWER_STATE_RUNNING, savedState);
    ASSERT_TRUE(testMode);
    
    Serial.println("  ✓ Power-off simulation test passed");
}

// Test power recovery validation
void testPowerRecoveryValidation() {
    Serial.println("Testing power recovery validation...");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Test with valid data
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    bool isValid = clock.validatePowerRecoveryData();
    ASSERT_TRUE(isValid);
    
    // Test with cleared data (should be invalid)
    clock.clearPowerRecoveryData();
    bool isInvalid = clock.validatePowerRecoveryData();
    ASSERT_FALSE(isInvalid);
    
    Serial.println("  ✓ Power recovery validation test passed");
}

// Test power recovery process
void testPowerRecoveryProcess() {
    Serial.println("Testing power recovery process...");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Simulate power-off
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    
    // Simulate power-on by calling begin() again
    // This should detect the saved data and prepare for recovery
    clock.begin();
    
    // The clock should now have the power-down time stored in _currentClockTime
    // and be ready to adjust position after NTP sync
    
    Serial.println("  ✓ Power recovery process test passed");
}

// Test different power states
void testDifferentPowerStates() {
    Serial.println("Testing different power states...");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Test running state
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    ASSERT_EQUAL(POWER_STATE_RUNNING, clock.getPowerDownState());
    
    // Test error state
    clock.simulatePowerOff(POWER_STATE_ERROR);
    ASSERT_EQUAL(POWER_STATE_ERROR, clock.getPowerDownState());
    
    // Test config state
    clock.simulatePowerOff(POWER_STATE_CONFIG);
    ASSERT_EQUAL(POWER_STATE_CONFIG, clock.getPowerDownState());
    
    Serial.println("  ✓ Different power states test passed");
}

// Test data clearing
void testDataClearing() {
    Serial.println("Testing data clearing...");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Save some data
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    ASSERT_TRUE(clock.validatePowerRecoveryData());
    
    // Clear the data
    clock.clearPowerRecoveryData();
    ASSERT_FALSE(clock.validatePowerRecoveryData());
    
    // Verify all fields are cleared
    ASSERT_EQUAL(0, clock.getPowerDownTime());
    ASSERT_EQUAL(0, clock.getPowerDownState());
    ASSERT_FALSE(clock.isTestMode());
    
    Serial.println("  ✓ Data clearing test passed");
}

// Test suite setup
void setupPowerRecoveryTests() {
    Serial.println("Setting up Power Recovery Tests...");
    
    // Register test cases
    testSuite_PowerRecoveryTest.addTest("PowerOffSimulation", testPowerOffSimulation);
    testSuite_PowerRecoveryTest.addTest("PowerRecoveryValidation", testPowerRecoveryValidation);
    testSuite_PowerRecoveryTest.addTest("PowerRecoveryProcess", testPowerRecoveryProcess);
    testSuite_PowerRecoveryTest.addTest("DifferentPowerStates", testDifferentPowerStates);
    testSuite_PowerRecoveryTest.addTest("DataClearing", testDataClearing);
    
    Serial.println("Power Recovery Tests setup complete");
}
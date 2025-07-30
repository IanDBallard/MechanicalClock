#include "MechanicalClock.h"
#include "TimeUtils.h"
#include <EEPROM.h>

// Simple power recovery test without complex test framework
void runPowerRecoveryTests() {
    Serial.println("=== POWER RECOVERY TEST SUITE ===");
    Serial.println("Testing improved power recovery system...");
    Serial.println("");
    
    // Initialize RTC first
    Serial.println("Initializing RTC...");
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    
    // Initialize RTC with a test time if not already set
    RTCTime testTime(1, Month::JANUARY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    rtcInstance.begin();
    rtcInstance.setTime(testTime);
    
    // Verify RTC is working
    RTCTime currentTime;
    rtcInstance.getTime(currentTime);
    time_t unixTime = currentTime.getUnixTime();
    Serial.print("RTC initialized. Current Unix time: ");
    Serial.println(unixTime);
    
    if (unixTime <= 0) {
        Serial.println("ERROR: RTC initialization failed!");
        return;
    }
    
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Test 1: Power-off simulation
    Serial.println("Test 1: Power-off simulation...");
    
    // Clear any existing data
    clock.clearPowerRecoveryData();
    
    // Simulate power-off in running state
    bool simulationResult = clock.simulatePowerOff(POWER_STATE_RUNNING);
    if (simulationResult) {
        Serial.println("  ✓ Power-off simulation successful");
    } else {
        Serial.println("  ✗ Power-off simulation failed");
        return;
    }
    
    // Verify data was saved
    time_t savedTime = clock.getPowerDownTime();
    uint8_t savedState = clock.getPowerDownState();
    bool testMode = clock.isTestMode();
    
    Serial.print("DEBUG: savedTime = "); Serial.println(savedTime);
    Serial.print("DEBUG: savedState = "); Serial.println(savedState);
    Serial.print("DEBUG: testMode = "); Serial.println(testMode);
    Serial.print("DEBUG: POWER_STATE_RUNNING = "); Serial.println(POWER_STATE_RUNNING);
    
    if (savedTime > 0 && savedState == POWER_STATE_RUNNING && testMode) {
        Serial.println("  ✓ Data verification passed");
    } else {
        Serial.println("  ✗ Data verification failed");
        Serial.print("    Expected: savedTime > 0, savedState = "); Serial.print(POWER_STATE_RUNNING); Serial.println(", testMode = true");
        Serial.print("    Got: savedTime = "); Serial.print(savedTime); 
        Serial.print(", savedState = "); Serial.print(savedState);
        Serial.print(", testMode = "); Serial.println(testMode);
        return;
    }
    
    // Test 2: Power recovery validation
    Serial.println("Test 2: Power recovery validation...");
    bool isValid = clock.validatePowerRecoveryData();
    if (isValid) {
        Serial.println("  ✓ Data validation passed");
    } else {
        Serial.println("  ✗ Data validation failed");
        return;
    }
    
    // Test 3: Data clearing
    Serial.println("Test 3: Data clearing...");
    clock.clearPowerRecoveryData();
    bool isInvalid = clock.validatePowerRecoveryData();
    if (!isInvalid) {
        Serial.println("  ✓ Data clearing successful");
    } else {
        Serial.println("  ✗ Data clearing failed");
        return;
    }
    
    // Test 4: Different power states
    Serial.println("Test 4: Different power states...");
    clock.simulatePowerOff(POWER_STATE_ERROR);
    if (clock.getPowerDownState() == POWER_STATE_ERROR) {
        Serial.println("  ✓ Error state test passed");
    } else {
        Serial.println("  ✗ Error state test failed");
        return;
    }
    
    clock.simulatePowerOff(POWER_STATE_CONFIG);
    if (clock.getPowerDownState() == POWER_STATE_CONFIG) {
        Serial.println("  ✓ Config state test passed");
    } else {
        Serial.println("  ✗ Config state test failed");
        return;
    }
    
    Serial.println("");
    Serial.println("=== ALL TESTS PASSED ===");
    Serial.println("Power recovery system is working correctly!");
}

// Setup function - called once at startup
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    runPowerRecoveryTests();
}

// Loop function - called repeatedly
void loop() {
    delay(1000);
    Serial.println("Arduino is still alive: " + String(millis()) + "ms");
}
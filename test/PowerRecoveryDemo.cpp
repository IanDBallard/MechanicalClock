#include "TestFramework.h"
#include "MechanicalClock.h"
#include "TimeUtils.h"
#include <EEPROM.h>

// Demonstration program for improved power recovery system
TestSuite testSuite_PowerRecoveryDemo("PowerRecoveryDemo");

// Demo: How to test power recovery without physical USB disconnection
void demoPowerRecoveryTesting() {
    Serial.println("=== POWER RECOVERY TESTING DEMO ===");
    Serial.println("This demo shows how to test power recovery without");
    Serial.println("physically disconnecting the USB cable.");
    Serial.println("");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Step 1: Initialize the clock normally
    Serial.println("Step 1: Initializing clock...");
    clock.begin();
    Serial.println("✓ Clock initialized");
    Serial.println("");
    
    // Step 2: Simulate a power-off event
    Serial.println("Step 2: Simulating power-off...");
    Serial.println("(This saves current time and state to EEPROM)");
    bool simulationResult = clock.simulatePowerOff(POWER_STATE_RUNNING);
    if (simulationResult) {
        Serial.println("✓ Power-off simulation successful");
    } else {
        Serial.println("✗ Power-off simulation failed");
        return;
    }
    Serial.println("");
    
    // Step 3: Verify the saved data
    Serial.println("Step 3: Verifying saved data...");
    time_t savedTime = clock.getPowerDownTime();
    uint8_t savedState = clock.getPowerDownState();
    bool testMode = clock.isTestMode();
    
    Serial.print("Saved time: "); Serial.println(savedTime);
    Serial.print("Saved state: "); Serial.println(savedState);
    Serial.print("Test mode: "); Serial.println(testMode ? "YES" : "NO");
    
    bool isValid = clock.validatePowerRecoveryData();
    Serial.print("Data validation: "); Serial.println(isValid ? "PASSED" : "FAILED");
    Serial.println("");
    
    // Step 4: Simulate power-on recovery
    Serial.println("Step 4: Simulating power-on recovery...");
    Serial.println("(This simulates what happens when power is restored)");
    
    // Create a new clock instance to simulate fresh boot
    LCDDisplay lcdDisplay2(0x27);
    RTClock rtcInstance2;
    MechanicalClock clock2(8, 7, 3, 4, 5, 6, 13, rtcInstance2, lcdDisplay2);
    
    // Call begin() - this should detect the saved data
    Serial.println("Calling clock.begin() to simulate power-on...");
    clock2.begin();
    
    Serial.println("✓ Power-on recovery simulation complete");
    Serial.println("");
    
    // Step 5: Clean up
    Serial.println("Step 5: Cleaning up test data...");
    clock2.clearPowerRecoveryData();
    Serial.println("✓ Test data cleared");
    Serial.println("");
    
    Serial.println("=== DEMO COMPLETE ===");
    Serial.println("This demonstrates how to test power recovery");
    Serial.println("without physical USB disconnection!");
}

// Demo: Testing different power-down scenarios
void demoDifferentScenarios() {
    Serial.println("=== DIFFERENT POWER-DOWN SCENARIOS DEMO ===");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Test power-down during running state
    Serial.println("Scenario 1: Power-down during normal running");
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    ASSERT_EQUAL(POWER_STATE_RUNNING, clock.getPowerDownState());
    Serial.println("✓ Running state power-down simulated");
    
    // Test power-down during error state
    Serial.println("Scenario 2: Power-down during error state");
    clock.simulatePowerOff(POWER_STATE_ERROR);
    ASSERT_EQUAL(POWER_STATE_ERROR, clock.getPowerDownState());
    Serial.println("✓ Error state power-down simulated");
    
    // Test power-down during configuration
    Serial.println("Scenario 3: Power-down during configuration");
    clock.simulatePowerOff(POWER_STATE_CONFIG);
    ASSERT_EQUAL(POWER_STATE_CONFIG, clock.getPowerDownState());
    Serial.println("✓ Configuration state power-down simulated");
    
    Serial.println("✓ All scenarios tested successfully");
}

// Demo: Data validation and corruption handling
void demoDataValidation() {
    Serial.println("=== DATA VALIDATION DEMO ===");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Test with valid data
    Serial.println("Test 1: Valid power recovery data");
    clock.simulatePowerOff(POWER_STATE_RUNNING);
    bool isValid = clock.validatePowerRecoveryData();
    ASSERT_TRUE(isValid);
    Serial.println("✓ Valid data test passed");
    
    // Test with cleared data
    Serial.println("Test 2: Cleared data (should be invalid)");
    clock.clearPowerRecoveryData();
    bool isInvalid = clock.validatePowerRecoveryData();
    ASSERT_FALSE(isInvalid);
    Serial.println("✓ Invalid data test passed");
    
    // Test with corrupted data
    Serial.println("Test 3: Corrupted data simulation");
    EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, (uint32_t)0x12345678); // Wrong magic number
    bool isCorrupted = clock.validatePowerRecoveryData();
    ASSERT_FALSE(isCorrupted);
    Serial.println("✓ Corrupted data test passed");
    
    Serial.println("✓ All validation tests passed");
}

// Demo: Interactive testing interface
void demoInteractiveTesting() {
    Serial.println("=== INTERACTIVE POWER RECOVERY TESTING ===");
    Serial.println("Commands:");
    Serial.println("  'sim' - Simulate power-off");
    Serial.println("  'rec' - Simulate power-on recovery");
    Serial.println("  'val' - Validate saved data");
    Serial.println("  'clr' - Clear saved data");
    Serial.println("  'info' - Show saved data info");
    Serial.println("  'help' - Show this help");
    Serial.println("  'quit' - Exit demo");
    Serial.println("");
    
    LCDDisplay lcdDisplay(0x27);
    RTClock rtcInstance;
    MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
    
    // Initialize clock
    clock.begin();
    
    while (true) {
        Serial.print("PowerRecovery> ");
        
        // Wait for input (simplified - in real implementation you'd read Serial)
        delay(1000); // Simulate waiting for input
        
        // For demo purposes, we'll just run through the commands
        Serial.println("sim");
        Serial.println("Simulating power-off...");
        clock.simulatePowerOff(POWER_STATE_RUNNING);
        Serial.println("✓ Power-off simulated");
        
        Serial.println("info");
        time_t savedTime = clock.getPowerDownTime();
        uint8_t savedState = clock.getPowerDownState();
        bool testMode = clock.isTestMode();
        Serial.print("Saved time: "); Serial.println(savedTime);
        Serial.print("Saved state: "); Serial.println(savedState);
        Serial.print("Test mode: "); Serial.println(testMode ? "YES" : "NO");
        
        Serial.println("val");
        bool isValid = clock.validatePowerRecoveryData();
        Serial.print("Validation: "); Serial.println(isValid ? "PASSED" : "FAILED");
        
        Serial.println("rec");
        Serial.println("Simulating power-on recovery...");
        LCDDisplay lcdDisplay2(0x27);
        RTClock rtcInstance2;
        MechanicalClock clock2(8, 7, 3, 4, 5, 6, 13, rtcInstance2, lcdDisplay2);
        clock2.begin();
        Serial.println("✓ Recovery simulated");
        
        Serial.println("clr");
        clock2.clearPowerRecoveryData();
        Serial.println("✓ Data cleared");
        
        Serial.println("quit");
        Serial.println("Exiting demo...");
        break;
    }
    
    Serial.println("=== INTERACTIVE DEMO COMPLETE ===");
}

// Demo setup
void setupPowerRecoveryDemo() {
    Serial.println("Setting up Power Recovery Demo...");
    
    // Register demo functions
    testSuite_PowerRecoveryDemo.addTest("PowerRecoveryTesting", demoPowerRecoveryTesting);
    testSuite_PowerRecoveryDemo.addTest("DifferentScenarios", demoDifferentScenarios);
    testSuite_PowerRecoveryDemo.addTest("DataValidation", demoDataValidation);
    testSuite_PowerRecoveryDemo.addTest("InteractiveTesting", demoInteractiveTesting);
    
    Serial.println("Power Recovery Demo setup complete");
}
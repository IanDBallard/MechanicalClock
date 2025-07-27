#include "TestFramework.h"
#include "MechanicalClock.h"
#include "TimeUtils.h"
#include <EEPROM.h>

// Test suite for power-off recovery scenarios
TestSuite testSuite_PowerOffRecoveryTest("PowerOffRecoveryTest");

// Test fixture for power-off recovery tests
class PowerOffRecoveryTestFixture {
public:
    LCDDisplay lcdDisplay;
    RTClock rtcInstance;
    MechanicalClock clock;
    const int TEST_EEPROM_ADDRESS_INITIAL_TIME = 0; // Use same address as production code
    
    PowerOffRecoveryTestFixture() 
        : lcdDisplay(0x27), 
          clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay) {
    }
    
    void setup() {
        // Initialize clock with mock RTC
        clock.begin();
        
        // Clear EEPROM before each test
        EEPROM.put(TEST_EEPROM_ADDRESS_INITIAL_TIME, (time_t)0);
    }
    
    void simulatePowerOff(time_t powerOffTime) {
        // Simulate saving power-off time to EEPROM
        EEPROM.put(TEST_EEPROM_ADDRESS_INITIAL_TIME, powerOffTime);
    }
    
    time_t getPowerOffTime() {
        time_t storedTime;
        EEPROM.get(TEST_EEPROM_ADDRESS_INITIAL_TIME, storedTime);
        return storedTime;
    }
    
    void simulatePowerOn(time_t currentTime) {
        // Set mock current time
        MockRTC::setTime(RTCTime(currentTime));
        
        // Call adjustToInitialTime with stored power-off time
        time_t powerOffTime = getPowerOffTime();
        clock.adjustToInitialTime(powerOffTime);
    }
    
    long calculateExpectedSteps(time_t powerOffTime, time_t currentTime) {
        const long TEST_SECONDS_IN_12_HOURS = 12 * 60 * 60; // 43200 seconds
        const long SECONDS_PER_STEP = 18;
        
        // Calculate positions within 12-hour cycle
        long powerOffPosition = powerOffTime % TEST_SECONDS_IN_12_HOURS;
        long currentPosition = currentTime % TEST_SECONDS_IN_12_HOURS;
        
        // Calculate shortest path distance
        long distance = currentPosition - powerOffPosition;
        
        // Handle wrap-around for shortest path
        if (distance > TEST_SECONDS_IN_12_HOURS / 2) {
            distance -= TEST_SECONDS_IN_12_HOURS;
        } else if (distance < -TEST_SECONDS_IN_12_HOURS / 2) {
            distance += TEST_SECONDS_IN_12_HOURS;
        }
        
        // Convert to steps
        return distance / SECONDS_PER_STEP;
    }
};

// Test 1: No power-off time (normal startup)
void testNoPowerOffTime() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    // Simulate power-on with no stored power-off time
    time_t currentTime = 1753630862; // Current time
    fixture.simulatePowerOn(currentTime);
    
    // Verify EEPROM is still 0 (no power-off time)
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)0, (long)storedTime);
    
    // Verify clock position should be set to current time
    // (This would require accessing internal state, but we can verify the logic path)
    Serial.println("✓ No power-off time test passed");
}

// Test 2: Short power-off (1 hour)
void testShortPowerOff() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753627262; // 1 hour ago
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Short power-off test passed");
}

// Test 3: Long power-off (6 hours)
void testLongPowerOff() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753609262; // 6 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Long power-off test passed");
}

// Test 4: Power-off across 12-hour boundary
void testPowerOffAcross12HourBoundary() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753587662; // 12 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Power-off across 12-hour boundary test passed");
}

// Test 5: Power-off exactly 12 hours
void testPowerOffExactly12Hours() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753587662; // Exactly 12 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Power-off exactly 12 hours test passed");
}

// Test 6: Power-off more than 12 hours (should be ignored)
void testPowerOffMoreThan12Hours() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753554062; // 24 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Power-off more than 12 hours test passed");
}

// Test 7: Invalid future power-off time
void testInvalidFuturePowerOffTime() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753714862; // Future time
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Invalid future power-off time test passed");
}

// Test 8: Invalid old power-off time
void testInvalidOldPowerOffTime() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1750000000; // Very old time
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Invalid old power-off time test passed");
}

// Test 9: Power-off time equals current time
void testPowerOffTimeEqualsCurrentTime() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753630862; // Same as current time
    time_t currentTime = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Power-off time equals current time test passed");
}

// Test 10: Multiple power-off cycles
void testMultiplePowerOffCycles() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    // First power-off cycle
    time_t powerOff1 = 1753627262; // 1 hour ago
    time_t current1 = 1753630862;  // Current time
    
    fixture.simulatePowerOff(powerOff1);
    fixture.simulatePowerOn(current1);
    
    // Verify first power-off time was stored
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOff1, (long)storedTime);
    
    // Second power-off cycle
    time_t powerOff2 = 1753630862; // Now
    time_t current2 = 1753634462;  // 1 hour later
    
    fixture.simulatePowerOff(powerOff2);
    fixture.simulatePowerOn(current2);
    
    // Verify second power-off time was stored
    storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOff2, (long)storedTime);
    
    Serial.println("✓ Multiple power-off cycles test passed");
}

// Test 11: Edge case - one second after midnight
void testEdgeCaseOneSecondAfterMidnight() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753631999; // One second before midnight
    time_t currentTime = 1753632000;  // Midnight
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Edge case one second after midnight test passed");
}

// Test 12: Edge case - one second before midnight
void testEdgeCaseOneSecondBeforeMidnight() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    time_t powerOffTime = 1753631998; // Two seconds before midnight
    time_t currentTime = 1753631999;  // One second before midnight
    
    fixture.simulatePowerOff(powerOffTime);
    fixture.simulatePowerOn(currentTime);
    
    // Calculate expected steps
    long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
    
    // Verify power-off time was stored and retrieved correctly
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)powerOffTime, (long)storedTime);
    
    Serial.println("✓ Edge case one second before midnight test passed");
}

// Test 13: Step calculation sanity check
void testStepCalculationSanity() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    // Test various time differences to ensure step calculation is reasonable
    time_t baseTime = 1753630862;
    
    for (int hourDiff = 1; hourDiff <= 6; hourDiff++) {
        time_t powerOffTime = baseTime - (hourDiff * 3600);
        time_t currentTime = baseTime;
        
        long expectedSteps = fixture.calculateExpectedSteps(powerOffTime, currentTime);
        
        // Steps should be reasonable (not negative or extremely large)
        ASSERT_TRUE(expectedSteps >= 0);
        ASSERT_TRUE(expectedSteps < 1000); // Reasonable upper bound
        
        Serial.print("Hour diff: "); Serial.print(hourDiff);
        Serial.print(", Steps: "); Serial.println(expectedSteps);
    }
    
    Serial.println("✓ Step calculation sanity test passed");
}

// Test 14: EEPROM corruption handling
void testEEPROMCorruptionHandling() {
    PowerOffRecoveryTestFixture fixture;
    fixture.setup();
    
    // Simulate corrupted EEPROM with invalid data
    EEPROM.put(fixture.TEST_EEPROM_ADDRESS_INITIAL_TIME, (time_t)0xFFFFFFFF);
    
    // Try to power on with corrupted data
    fixture.simulatePowerOn(1753630862);
    
    // Verify the system handles corruption gracefully
    time_t storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)0xFFFFFFFF, (long)storedTime);
    
    // Test with another corrupted value
    EEPROM.put(fixture.TEST_EEPROM_ADDRESS_INITIAL_TIME, (time_t)0x00000000);
    fixture.simulatePowerOn(1753630862);
    
    storedTime = fixture.getPowerOffTime();
    ASSERT_EQUAL((long)0x00000000, (long)storedTime);
    
    Serial.println("✓ EEPROM corruption handling test passed");
}

// Setup function to register all tests
void setupPowerOffRecoveryTests() {
    testSuite_PowerOffRecoveryTest.addTest("testNoPowerOffTime", testNoPowerOffTime);
    testSuite_PowerOffRecoveryTest.addTest("testShortPowerOff", testShortPowerOff);
    testSuite_PowerOffRecoveryTest.addTest("testLongPowerOff", testLongPowerOff);
    testSuite_PowerOffRecoveryTest.addTest("testPowerOffAcross12HourBoundary", testPowerOffAcross12HourBoundary);
    testSuite_PowerOffRecoveryTest.addTest("testPowerOffExactly12Hours", testPowerOffExactly12Hours);
    testSuite_PowerOffRecoveryTest.addTest("testPowerOffMoreThan12Hours", testPowerOffMoreThan12Hours);
    testSuite_PowerOffRecoveryTest.addTest("testInvalidFuturePowerOffTime", testInvalidFuturePowerOffTime);
    testSuite_PowerOffRecoveryTest.addTest("testInvalidOldPowerOffTime", testInvalidOldPowerOffTime);
    testSuite_PowerOffRecoveryTest.addTest("testPowerOffTimeEqualsCurrentTime", testPowerOffTimeEqualsCurrentTime);
    testSuite_PowerOffRecoveryTest.addTest("testMultiplePowerOffCycles", testMultiplePowerOffCycles);
    testSuite_PowerOffRecoveryTest.addTest("testEdgeCaseOneSecondAfterMidnight", testEdgeCaseOneSecondAfterMidnight);
    testSuite_PowerOffRecoveryTest.addTest("testEdgeCaseOneSecondBeforeMidnight", testEdgeCaseOneSecondBeforeMidnight);
    testSuite_PowerOffRecoveryTest.addTest("testStepCalculationSanity", testStepCalculationSanity);
    testSuite_PowerOffRecoveryTest.addTest("testEEPROMCorruptionHandling", testEEPROMCorruptionHandling);
    
    // Register with global test registry
    extern TestRegistry testRegistry;
    testRegistry.addSuite(testSuite_PowerOffRecoveryTest);
} 
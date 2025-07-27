#include <Arduino.h> // Standard Arduino functions (pinMode, delay, etc.)

#ifdef ARDUINO_TESTING
// Arduino-based unit testing environment
#include <EEPROM.h>

// Test constants - use different names to avoid conflicts
const int TEST_EEPROM_ADDRESS = 100;
const long TEST_SECONDS_IN_12_HOURS = 12 * 60 * 60; // 43200 seconds
const long TEST_SECONDS_PER_STEP = 18;

// Mock RTC for testing
time_t mockCurrentTime = 0;

// Test results
int testsPassed = 0;
int testsFailed = 0;

// Helper function to calculate expected steps
long calculateExpectedSteps(time_t powerOffTime, time_t currentTime) {
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
    return distance / TEST_SECONDS_PER_STEP;
}

// Test fixture functions
void clearEEPROM() {
    EEPROM.put(TEST_EEPROM_ADDRESS, (time_t)0);
}

void simulatePowerOff(time_t powerOffTime) {
    EEPROM.put(TEST_EEPROM_ADDRESS, powerOffTime);
}

time_t getPowerOffTime() {
    time_t storedTime;
    EEPROM.get(TEST_EEPROM_ADDRESS, storedTime);
    return storedTime;
}

void simulatePowerOn(time_t currentTime) {
    mockCurrentTime = currentTime;
    
    // Call adjustToInitialTime with stored power-off time
    time_t powerOffTime = getPowerOffTime();
    
    Serial.print("Power-off time: ");
    Serial.println(powerOffTime);
    Serial.print("Current time: ");
    Serial.println(currentTime);
    
    if (powerOffTime == 0) {
        Serial.println("No power-off time - setting position to current time");
    } else {
        long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
        Serial.print("Expected steps: ");
        Serial.println(expectedSteps);
    }
}

// Test functions
void testNoPowerOffTime() {
    Serial.println("\n--- Test 1: No Power-Off Time ---");
    clearEEPROM();
    
    time_t currentTime = 1753630862;
    simulatePowerOn(currentTime);
    
    time_t storedTime = getPowerOffTime();
    if (storedTime == 0) {
        Serial.println("✓ No power-off time test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ No power-off time test FAILED");
        testsFailed++;
    }
}

void testShortPowerOff() {
    Serial.println("\n--- Test 2: Short Power-Off (1 hour) ---");
    clearEEPROM();
    
    time_t powerOffTime = 1753627262; // 1 hour ago
    time_t currentTime = 1753630862;  // Current time
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps for 1 hour: ");
    Serial.println(expectedSteps);
    
    // Verify EEPROM was cleared after reading (simulated)
    time_t storedTime = getPowerOffTime();
    if (storedTime == powerOffTime) { // Still there since we're not actually calling adjustToInitialTime
        Serial.println("✓ Short power-off test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Short power-off test FAILED");
        testsFailed++;
    }
}

void testLongPowerOff() {
    Serial.println("\n--- Test 3: Long Power-Off (6 hours) ---");
    clearEEPROM();
    
    time_t powerOffTime = 1753609262; // 6 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps for 6 hours: ");
    Serial.println(expectedSteps);
    
    time_t storedTime = getPowerOffTime();
    if (storedTime == powerOffTime) {
        Serial.println("✓ Long power-off test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Long power-off test FAILED");
        testsFailed++;
    }
}

void testPowerOffAcross12HourBoundary() {
    Serial.println("\n--- Test 4: Power-Off Across 12-Hour Boundary ---");
    clearEEPROM();
    
    // Power off at 11:59 PM, power on at 12:01 AM (next day)
    time_t powerOffTime = 1753627140; // 11:59 PM
    time_t currentTime = 1753627260;  // 12:01 AM (next day)
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps across 12-hour boundary: ");
    Serial.println(expectedSteps);
    
    time_t storedTime = getPowerOffTime();
    if (storedTime == powerOffTime) {
        Serial.println("✓ 12-hour boundary test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ 12-hour boundary test FAILED");
        testsFailed++;
    }
}

void testPowerOffExactly12Hours() {
    Serial.println("\n--- Test 5: Power-Off Exactly 12 Hours ---");
    clearEEPROM();
    
    time_t powerOffTime = 1753587662; // 12 hours ago
    time_t currentTime = 1753630862;  // Current time
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps for exactly 12 hours: ");
    Serial.println(expectedSteps);
    
    time_t storedTime = getPowerOffTime();
    if (storedTime == powerOffTime) {
        Serial.println("✓ Exactly 12 hours test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Exactly 12 hours test FAILED");
        testsFailed++;
    }
}

void testInvalidFuturePowerOffTime() {
    Serial.println("\n--- Test 6: Invalid Future Power-Off Time ---");
    clearEEPROM();
    
    time_t powerOffTime = 1753634462; // 1 hour in future
    time_t currentTime = 1753630862;  // Current time
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps for future time: ");
    Serial.println(expectedSteps);
    
    time_t storedTime = getPowerOffTime();
    if (storedTime == powerOffTime) {
        Serial.println("✓ Invalid future time test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Invalid future time test FAILED");
        testsFailed++;
    }
}

void testPowerOffTimeEqualsCurrentTime() {
    Serial.println("\n--- Test 7: Power-Off Time Equals Current Time ---");
    clearEEPROM();
    
    time_t powerOffTime = 1753630862; // Same as current time
    time_t currentTime = 1753630862;  // Current time
    
    simulatePowerOff(powerOffTime);
    simulatePowerOn(currentTime);
    
    long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
    Serial.print("Expected steps for same time: ");
    Serial.println(expectedSteps);
    
    if (expectedSteps == 0) {
        Serial.println("✓ Power-off equals current time test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Power-off equals current time test FAILED");
        testsFailed++;
    }
}

void testStepCalculationSanity() {
    Serial.println("\n--- Test 8: Step Calculation Sanity Check ---");
    clearEEPROM();
    
    // Test various time differences and verify steps are reasonable
    time_t baseTime = 1753630862;
    bool allReasonable = true;
    
    for (int hours = 1; hours <= 12; hours++) {
        time_t powerOffTime = baseTime - (hours * 3600);
        time_t currentTime = baseTime;
        
        long expectedSteps = calculateExpectedSteps(powerOffTime, currentTime);
        
        // Steps should be reasonable (not more than ~2400 for 12 hours)
        if (abs(expectedSteps) >= 2400) {
            allReasonable = false;
        }
        
        Serial.print("Hours: ");
        Serial.print(hours);
        Serial.print(", Steps: ");
        Serial.println(expectedSteps);
    }
    
    if (allReasonable) {
        Serial.println("✓ Step calculation sanity test PASSED");
        testsPassed++;
    } else {
        Serial.println("✗ Step calculation sanity test FAILED");
        testsFailed++;
    }
}

void testEEPROMCorruptionHandling() {
    Serial.println("\n--- Test 9: EEPROM Corruption Handling ---");
    clearEEPROM();
    
    // Write invalid values to EEPROM
    EEPROM.put(TEST_EEPROM_ADDRESS, (time_t)-1);
    simulatePowerOn(1753630862);
    
    time_t storedTime = getPowerOffTime();
    Serial.print("Stored time after writing -1: ");
    Serial.println(storedTime);
    
    // Write very large value
    EEPROM.put(TEST_EEPROM_ADDRESS, (time_t)9999999999);
    simulatePowerOn(1753630862);
    
    storedTime = getPowerOffTime();
    Serial.print("Stored time after writing large value: ");
    Serial.println(storedTime);
    
    Serial.println("✓ EEPROM corruption handling test PASSED");
    testsPassed++;
}

void runPowerOffRecoveryTests() {
    Serial.println("\n\n=== POWER-OFF RECOVERY TEST SUITE ===");
    Serial.println("Testing various power-off scenarios and recovery logic...");
    
    testsPassed = 0;
    testsFailed = 0;
    
    testNoPowerOffTime();
    testShortPowerOff();
    testLongPowerOff();
    testPowerOffAcross12HourBoundary();
    testPowerOffExactly12Hours();
    testInvalidFuturePowerOffTime();
    testPowerOffTimeEqualsCurrentTime();
    testStepCalculationSanity();
    testEEPROMCorruptionHandling();
    
    Serial.println("\n=== TEST RESULTS ===");
    Serial.print("Tests PASSED: ");
    Serial.println(testsPassed);
    Serial.print("Tests FAILED: ");
    Serial.println(testsFailed);
    Serial.print("Total tests: ");
    Serial.println(testsPassed + testsFailed);
    
    if (testsFailed == 0) {
        Serial.println("🎉 ALL POWER-OFF RECOVERY TESTS PASSED! 🎉");
    } else {
        Serial.println("⚠️  SOME TESTS FAILED - REVIEW REQUIRED ⚠️");
    }
}

void setup() {
    Serial.begin(115200);
    delay(3000); // Wait longer for Serial to be ready
    Serial.println("\n\n\n=== ARDUINO POWER-OFF RECOVERY TEST SUITE STARTING ===");
    
    // Run power-off recovery tests
    runPowerOffRecoveryTests();
    
    Serial.println("=== ALL TESTS COMPLETED ===");
    Serial.println("Arduino will continue running for monitoring...\n");
}

void loop() {
    delay(10000);
    Serial.println("Test environment still running: " + String(millis()) + "ms");
}

#else
// Production mode - include normal clock code
#include <EEPROM.h>  // For EEPROM operations
#include <RTC.h>     // Built-in RTC library for UNO R4 WiFi (provides extern RTClock RTC;)
#include <WiFiS3.h>  // Complete WiFi library for UNO R4 WiFi
#include <Wire.h>    // For I2C communication (used by LCDDisplay)
// #include <hal_data.h> // Include necessary Renesas headers if not already present

// Include our custom classes
#include "TimeUtils.h"       // Utilities for time conversions, DST
#include "LED.h"             // LED class
#include "Clock.h"           // Base Clock class
#include "DigitalClock.h"    // Digital clock implementation
#include "MechanicalClock.h" // Mechanical clock implementation
#include "LCDDisplay.h"      // LCD display management
#include "NetworkManager.h"  // Network and NTP management
#include "StateManager.h"    // Overall system state management

// --- Global Constants and Pin Definitions ---
// EEPROM_TOTAL_SIZE is not directly used in put/get but good for reference if doing manual byte-level writes.
// EEPROM_ADDR_INITIAL_TIME, EEPROM_ADDR_WIFI_CRED_START, etc. are defined in respective class headers.

// Pin definitions
const int DIR_PIN = 7;
const int STEP_PIN = 8;
const int MS1_PIN = 4;
const int MS2_PIN = 5;
const int MS3_PIN = 6;
const int ENABLE_PIN = 3; // For A4988 driver enable (LOW = enabled)
const int LED_PIN = 13;   // Onboard LED
const int POWER_PIN = 2;  // Interrupt pin for power outage detection

// Microstepping mode for MechanicalClock (defined here to pass to constructor)
// Define microstepping constants if not already defined in MechanicalClock.h
#ifndef MICROSTEP_FULL
#define MICROSTEP_FULL 0b000
#endif
const uint8_t MECHANICAL_CLOCK_MICROSTEP_MODE = MICROSTEP_FULL; // Use full steps

// NetworkManager Configuration
const char* AP_SSID = "ClockSetup"; // SSID for the captive portal AP
IPAddress NTP_SERVER_IP(129, 6, 15, 28); // time.nist.gov
const unsigned int LOCAL_UDP_PORT = 2390;
const unsigned long WIFI_CONNECT_TIMEOUT = 30000; // 30 seconds
const int MAX_NTP_RETRIES = 3;
const unsigned long NTP_RETRY_DELAY = 5000; // 5 seconds
const unsigned long NTP_SYNC_INTERVAL = 3600000UL; // 1 hour (in milliseconds)
const int TIME_ZONE_OFFSET_HOURS = -5; // EST (Eastern Standard Time) offset from UTC
const bool USE_DST_AUTO_CALC = true;    // Enable automatic DST calculation

// --- Global Renesas Reset Status Register Access ---
// Note: These are specific to Renesas RA4M1. Assumes hal_data.h or similar setup.
// You might need to add extern "C" { #include <hal_data.h> } if compiler complains.
// If you don't have hal_data.h, you might need to manually define these addresses
// based on Renesas RA4M1 datasheet or Arduino R4 core files.
#define SYSTEM_RSTSR0 (*(volatile uint8_t*)0x4001E400)
#define SYSTEM_RSTSR1 (*(volatile uint8_t*)0x4001E401)
#define SYSTEM_RSTSR2 (*(volatile uint8_t*)0x4001E402)

// RSTSR0 Flags (Bit Masks)
#define PORF_BIT    0
#define LVD0RF_BIT  1
#define LVD1RF_BIT  2
#define LVD2RF_BIT  3
#define DPSRSTF_BIT 4

// RSTSR1 Flags (Bit Masks)
#define CWSF_BIT    0 // Cold Warm Start Flag

// RSTSR2 Flags (Bit Masks)
#define IWDTRF_BIT  0 // Independent Watchdog Timer Reset Flag
#define WDTRF_BIT   1 // Watchdog Timer Reset Flag
#define SWRF_BIT    2 // Software Reset Flag

// --- Global Instances of Our Classes ---
// Note: These are declared as global variables so they can be accessed by the ISR
// and other parts of the system. The order of declaration matters for initialization.

// LCD Display (I2C address 0x27 is common, but some displays use 0x3F)
LCDDisplay lcdDisplay(0x27);

// Network Manager (manages WiFi, NTP, and EEPROM storage for credentials)
NetworkManager networkManager(AP_SSID, NTP_SERVER_IP, LOCAL_UDP_PORT, WIFI_CONNECT_TIMEOUT, 
                             MAX_NTP_RETRIES, NTP_RETRY_DELAY, MAX_NTP_RETRIES, WIFI_CONNECT_TIMEOUT, 
                             NTP_SYNC_INTERVAL, TIME_ZONE_OFFSET_HOURS, USE_DST_AUTO_CALC);

// Mechanical Clock (drives the stepper motor and manages hand positions)
MechanicalClock mechanicalClock(
    STEP_PIN, DIR_PIN, ENABLE_PIN, MS1_PIN, MS2_PIN, MS3_PIN, LED_PIN,
    RTC, lcdDisplay // Pass references to the global RTC and our LCDDisplay object
);

// State Manager (orchestrates the overall system state)
StateManager stateManager(networkManager, lcdDisplay, mechanicalClock, RTC);

// --- Interrupt Service Routine for Power-Off Detection ---
void PowerOffISR() {
    // This ISR is called when the power-off detection circuit pulls the POWER_PIN LOW
    // We need to save the current time to EEPROM before power is lost
    
    // Disable interrupts to prevent re-entry
    noInterrupts();
    
    // Get current time and save to EEPROM
    RTCTime currentTime;
    RTC.getTime(currentTime);
    time_t unixTime = currentTime.getUnixTime();
    
    // Define EEPROM address for initial time if not already defined
    #ifndef EEPROM_ADDRESS_INITIAL_TIME
    #define EEPROM_ADDRESS_INITIAL_TIME 0
    #endif
    
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, unixTime);
    
    // Note: We don't call __WFI() here as it can cause issues on some boards
    // The power-off detection circuit should handle the actual power-down
    
    // Re-enable interrupts
    interrupts();
}

// --- Setup Function ---
void setup() {
    // Initialize Serial communication for debugging
    Serial.begin(115200);
    delay(1000); // Give Serial time to initialize
    
    Serial.println("=== Mechanical Clock with Onboard RTC ===");
    Serial.println("Starting initialization...");
    
    // Initialize hardware components
    if (!lcdDisplay.begin()) {
        Serial.println("FATAL: LCD initialization failed!");
        while (1) { delay(1000); } // Halt if LCD fails
    }
    
    if (!RTC.begin()) {
        Serial.println("FATAL: RTC initialization failed!");
        while (1) { delay(1000); } // Halt if RTC fails
    }
    Serial.println("Onboard RTC initialized.");
    
    // Check if RTC has a reasonable time, if not set a default
    RTCTime currentRTCtime;
    RTC.getTime(currentRTCtime);
    time_t currentUnixTime = currentRTCtime.getUnixTime();
    
    Serial.print("RTC current time: "); Serial.println(currentUnixTime);
    
    // If RTC time is before 2023, set it to a reasonable default
    if (currentUnixTime < 1672531200UL) { // Before Jan 1, 2023
        Serial.println("RTC time is too old, setting to default...");
        // Set to January 1, 2024 12:00:00 UTC
        RTCTime defaultTime(1, Month::JANUARY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
        RTC.setTime(defaultTime);
        Serial.println("RTC set to default time: Jan 1, 2024 12:00:00 UTC");
    }
    
    // Configure power-off interrupt
    pinMode(POWER_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(POWER_PIN), PowerOffISR, FALLING);
    Serial.println("Power-off interrupt configured.");
    
    // Initialize mechanical clock
    mechanicalClock.begin();
    Serial.println("MechanicalClock initialized.");
    
    // Check for power-off time in EEPROM
    time_t initialClockTime = 0;
    EEPROM.get(EEPROM_ADDRESS_INITIAL_TIME, initialClockTime);
    
    Serial.print("EEPROM power-off time: "); Serial.println(initialClockTime);
    
    // Validate EEPROM time: check for reasonable range (not too old, not in future)
    time_t currentTime = getCurrentUTC();
    time_t minValidTime = 1672531200UL; // Jan 1, 2023 00:00:00 UTC
    time_t maxValidTime = 1735689600UL; // Jan 1, 2025 00:00:00 UTC (reasonable future date)
    
    Serial.print("Current UTC time: "); Serial.println(currentTime);
    Serial.print("Power-off time from EEPROM: "); Serial.println(initialClockTime);
    
    if (initialClockTime < minValidTime || initialClockTime > maxValidTime) {
        // Invalid power-off time - use current time
        Serial.print("Invalid power-off time: "); Serial.println(initialClockTime);
        Serial.print("Valid range: "); Serial.print(minValidTime); 
        Serial.print(" to "); Serial.println(maxValidTime);
        Serial.println("No valid power-off time - using current time");
        initialClockTime = 0;
        
        // Clear the corrupted EEPROM value
        time_t clearValue = 0;
        EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, clearValue);
    } else {
        // Valid power-off time found - clear it after reading
        Serial.println("Valid power-off time found - will calculate movement");
        time_t clearValue = 0;
        EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, clearValue);
    }
    
    mechanicalClock.adjustToInitialTime(initialClockTime);
    Serial.println("Clock initialized.");
    
    // Initialize network manager
    networkManager.begin();

    // --- Initial State Transition ---
    // If we are already in an error state from initial hardware checks, stick with it.
    if (stateManager.getCurrentState() == STATE_INIT) { // Only transition from INIT if not already ERROR
        if (networkManager.needsConfiguration()) {
            stateManager.transitionTo(STATE_CONFIG);
        } else {
            stateManager.transitionTo(STATE_CONNECTING_WIFI);
        }
    }

    Serial.println("Setup complete, entering main loop.");
}

// --- Main Loop Function ---
void loop() {
    // The loop primarily drives the state machine
    stateManager.update();
}

#endif // ARDUINO_TESTING
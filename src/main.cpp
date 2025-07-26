#include <Arduino.h> // Standard Arduino functions (pinMode, delay, etc.)
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
const int TIME_ZONE_OFFSET_HOURS = -4; // EDT (Eastern Daylight Time) offset from UTC
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
// The RTC object is externally provided by RTC.h
extern RTClock RTC; // Access the global onboard RTC instance

LCDDisplay lcdDisplay(0x27); // LCD address (adjust if yours is different, e.g., 0x3F)

// NetworkManager instance, passed its configuration
NetworkManager networkManager(
    AP_SSID,
    NTP_SERVER_IP,
    LOCAL_UDP_PORT,
    WIFI_CONNECT_TIMEOUT,
    MAX_NTP_RETRIES,
    NTP_RETRY_DELAY,
    0, // wifiReconnectRetries - not fully implemented yet, use 0
    0, // wifiReconnectDelay - not fully implemented yet, use 0
    NTP_SYNC_INTERVAL,
    TIME_ZONE_OFFSET_HOURS,
    USE_DST_AUTO_CALC
);

// MechanicalClock instance, passed its pins and dependencies
MechanicalClock mechanicalClock(
    STEP_PIN, DIR_PIN, ENABLE_PIN, MS1_PIN, MS2_PIN, MS3_PIN, LED_PIN,
    RTC, lcdDisplay // Pass references to the global RTC and our LCDDisplay object
);

// StateManager instance, orchestrates other managers
StateManager stateManager(networkManager, lcdDisplay, mechanicalClock, RTC);

// --- Interrupt Service Routine (ISR) ---
// This must be a global function or a static class method
void PowerOffISR() {
    // Transition to Power Saving state, and let mechanicalClock handle its power-off logic
    // ISRs must be minimal: no Serial.print(), no delay(), no complex operations.
    // The state transition and mechanicalClock.handlePowerOff() must be ISR-safe.
    // (We designed handlePowerOff to be minimal: EEPROM.put and digitalWrite).
    stateManager.transitionTo(STATE_POWER_SAVING); // Signal state change (fast)
    mechanicalClock.handlePowerOff();              // Perform critical power-off actions (fast)
    // __WFI(); // Enter low power mode (Wait For Interrupt) - Commented out as it may not be available in Arduino environment
    // For Arduino compatibility, we'll let the ISR return normally
}

// --- Setup Function ---
void setup() {
    Serial.begin(9600); // Start serial communication very early

    // --- EEPROM Initialization ---
    // EEPROM.begin() is not explicitly needed for R4's EEPROM library.

    // --- Reset Cause Detection ---
    // Read reset status registers IMMEDIATELY (reading clears flags)
    uint8_t rstsr0_val = SYSTEM_RSTSR0;
    uint8_t rstsr1_val = SYSTEM_RSTSR1;
    uint8_t rstsr2_val = SYSTEM_RSTSR2;

    Serial.println("\n\n=== Clock System Booting ===");
    Serial.print("RSTSR0: 0b"); Serial.println(rstsr0_val, BIN);
    Serial.print("RSTSR1: 0b"); Serial.println(rstsr1_val, BIN);
    Serial.print("RSTSR2: 0b"); Serial.println(rstsr2_val, BIN);

    bool powerRelatedReset = bitRead(rstsr0_val, PORF_BIT)  ||
                             bitRead(rstsr0_val, LVD0RF_BIT) ||
                             bitRead(rstsr0_val, LVD1RF_BIT) ||
                             bitRead(rstsr0_val, LVD2RF_BIT) ||
                             bitRead(rstsr0_val, DPSRSTF_BIT);

    bool softOrWatchdogReset = bitRead(rstsr2_val, SWRF_BIT)   ||
                               bitRead(rstsr2_val, IWDTRF_BIT) ||
                               bitRead(rstsr2_val, WDTRF_BIT);

    bool isExternalReset = bitRead(rstsr1_val, CWSF_BIT); // CWSF is set on External Reset, not cleared by other flags
    
    if (powerRelatedReset) {
        Serial.println("Reset Cause: Power-On / Low Voltage / Deep Sleep Exit.");
    } else if (softOrWatchdogReset) {
        Serial.println("Reset Cause: Software / Watchdog Reset.");
    } else if (isExternalReset) { // External Reset (RESET button, IDE upload, debugger detach)
        Serial.println("Reset Cause: External Reset (Pin/Upload/Debug).");
    } else {
        Serial.println("Reset Cause: Unknown or Initial Cold Boot (No specific flags set).");
    }
    
    // --- Hardware Initialization ---
    Serial.println("Initializing core hardware...");
    
    // LCD Initialization (includes Wire.begin() internally)
    if (!lcdDisplay.begin()) {
        Serial.println("ERROR: LCD display initialization failed. Check wiring/address.");
        stateManager.setLastError("LCD Fail"); // Set error message
        stateManager.transitionTo(STATE_ERROR); // Transition to error state
        // If LCD is critical and error state can't be displayed, consider halting.
    }
    // Initial LCD message for setup is handled by StateManager::transitionTo(STATE_INIT)
    // or by subsequent state transitions.

    // RTC Initialization
    if (!RTC.begin()) {
        Serial.println("ERROR: Onboard RTC failed to initialize.");
        stateManager.setLastError("RTC Fail");
        stateManager.transitionTo(STATE_ERROR);
    } else {
        Serial.println("Onboard RTC initialized.");
    }
    
    // Configure Power Interrupt Pin and Attach ISR
    pinMode(POWER_PIN, INPUT_PULLUP); // Use INPUT_PULLUP if the detection circuit pulls LOW on power loss
    attachInterrupt(digitalPinToInterrupt(POWER_PIN), PowerOffISR, FALLING); // Trigger on falling edge
    Serial.println("Power-off interrupt configured.");

    // --- MechanicalClock Initialization ---
    mechanicalClock.begin(); // Initializes stepper pins, microstepping, etc.
    Serial.println("MechanicalClock initialized.");

    // Determine initial clock time source based on reset cause
    time_t initialClockTime = 0; // Default to epoch 0
    bool useEEPROMForTime = powerRelatedReset; // Use EEPROM time if power-related reset

    if (useEEPROMForTime) {
        // Define EEPROM address for initial time if not already defined
        #ifndef EEPROM_ADDRESS_INITIAL_TIME
        #define EEPROM_ADDRESS_INITIAL_TIME 0
        #endif
        EEPROM.get(EEPROM_ADDRESS_INITIAL_TIME, initialClockTime);
        Serial.print("Attempting to load time from EEPROM: "); Serial.println(initialClockTime);
        // Validate EEPROM time: a Unix timestamp for a reasonable year (e.g., 2023-01-01)
        if (initialClockTime < 1672531200UL) { // 1672531200 is Jan 1, 2023 00:00:00 UTC
            Serial.println("EEPROM time invalid/unset, falling back to current RTC time.");
            RTCTime tempNow;
            RTC.getTime(tempNow);
            initialClockTime = tempNow.getUnixTime();
        } else {
            // Clear the saved time immediately after reading it to avoid re-using old time if power fluctuates
            time_t clearValue = 0;
            EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, clearValue);
            Serial.println("✓ Cleared saved power-down time from EEPROM.");
        }
    } else { // Soft reset, watchdog, external reset, or first cold boot
        RTCTime tempNow;
        RTC.getTime(tempNow); // Get current time from RTC
        initialClockTime = tempNow.getUnixTime();
        Serial.print("Initial time from RTC: "); Serial.println(initialClockTime);
    }
    mechanicalClock.adjustToInitialTime(initialClockTime); // Set clock hands to this initial time
    Serial.print("Clock hands initialized to Unix time: "); Serial.println(initialClockTime);

    // --- NetworkManager Initialization ---
    // NetworkManager's begin() will load credentials and timezone from EEPROM
    networkManager.begin(); 
    Serial.println("NetworkManager initialized.");

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
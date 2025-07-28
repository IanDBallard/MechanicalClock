#include <Arduino.h> // Standard Arduino functions (pinMode, delay, etc.)

#ifdef ARDUINO_TESTING
// Simple test to see if Arduino is working
void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for Serial to be ready
    Serial.println("=== ARDUINO TEST STARTING ===");
    Serial.println("If you see this, the Arduino is working!");
    Serial.println("Current time: " + String(millis()) + "ms");
    Serial.println("=== TEST COMPLETE ===");
}

void loop() {
    delay(5000);
    Serial.println("Arduino is still alive: " + String(millis()) + "ms");
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
const int TIME_ZONE_OFFSET_HOURS = -4; // EDT (Eastern Daylight Time) offset from UTC
const bool USE_DST_AUTO_CALC = true;    // Enable automatic DST calculation

// --- Global Renesas Reset Status Register Access ---
// Note: These are specific to Renesas RA4M1. Assumes hal_data.h or similar setup.
// You might need to add extern "C" { #include <hal_data.h> } if compiler complains.
// If you don't have hal_data.h, you might need to manually define these addresses
// based on Renesas RA4M1 datasheet or Arduino R4 core files.




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
    Serial.println("Initializing system...");
    

    
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

#endif // ARDUINO_TESTING
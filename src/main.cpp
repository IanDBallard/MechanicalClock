/*
 * Mechanical Clock with Onboard RTC - Main Application
 * Copyright (C) 2024 iball
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
#include "Constants.h"       // Centralized constants

// --- Global Constants and Pin Definitions ---
// Pin definitions (all in Constants.h)
// const int DIR_PIN = 7;
// const int STEP_PIN = 8;
// const int MS1_PIN = 4;
// const int MS2_PIN = 5;
// const int MS3_PIN = 6;
// const int ENABLE_PIN = 3; // For A4988 driver enable (LOW = enabled)
// const int LED_PIN = 13;   // Onboard LED
// const int POWER_PIN = 2;  // Interrupt pin for power outage detection

// Microstepping mode for MechanicalClock (defined here to pass to constructor)
#ifndef MICROSTEP_FULL
#define MICROSTEP_FULL 0b000
#endif
const uint8_t MECHANICAL_CLOCK_MICROSTEP_MODE = MICROSTEP_FULL; // Use full steps

// NetworkManager Configuration (all in Constants.h)
// const char* AP_SSID = "ClockSetup"; // SSID for the captive portal AP
// IPAddress NTP_SERVER_IP(129, 6, 15, 28); // time.nist.gov
// const unsigned int LOCAL_UDP_PORT = 2390;
// const unsigned long WIFI_CONNECT_TIMEOUT = 30000; // 30 seconds
// const int MAX_NTP_RETRIES = 3;
// const unsigned long NTP_RETRY_DELAY = 5000; // 5 seconds
// const unsigned long NTP_SYNC_INTERVAL = 3600000UL; // 1 hour (in milliseconds)
// const int TIME_ZONE_OFFSET_HOURS = -4; // EDT (Eastern Daylight Time) offset from UTC
// const bool USE_DST_AUTO_CALC = true;    // Enable automatic DST calculation

// --- Global Instances of Our Classes ---
// Note: These are declared as global variables so they can be accessed by the ISR
// and other parts of the system. The order of declaration matters for initialization.

// LCD Display (I2C address 0x27 is common, but some displays use 0x3F)
LCDDisplay lcdDisplay(0x27);

// Network Manager (manages WiFi, NTP, and EEPROM storage for credentials)
// Parameters: (apSsid, ntpServerIP, localPort, wifiConnectTimeout, maxNtpRetries, ntpRetryDelay, wifiReconnectRetries, wifiReconnectDelay, ntpSyncInterval, timeZoneOffsetHours, useDST)
NetworkManager networkManager(AP_SSID, IPAddress(129, 6, 15, 28), 2390, WIFI_CONNECT_TIMEOUT, 3, 5000, 3, 10000, NTP_SYNC_INTERVAL, -4, true);

// Mechanical Clock (drives the stepper motor and manages hand positions)
MechanicalClock mechanicalClock(
    STEP_PIN, DIR_PIN, ENABLE_PIN, MS1_PIN, MS2_PIN, MS3_PIN, LED_PIN,
    RTC, lcdDisplay // Pass references to the global RTC and our LCDDisplay object
);

// State Manager (orchestrates the overall system state)
StateManager stateManager(networkManager, lcdDisplay, mechanicalClock, RTC);

// --- Interrupt Service Routine for Power-Off Detection ---
void PowerOffISR() {
    // ISR: Save current time to EEPROM before power is lost (ISR-safe only)
    noInterrupts();
    RTCTime currentTime;
    RTC.getTime(currentTime);
    time_t unixTime = currentTime.getUnixTime();
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, unixTime);
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
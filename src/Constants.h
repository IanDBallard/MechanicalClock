#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============================================================================
// EEPROM ADDRESSES
// ============================================================================
// Centralized EEPROM address definitions to prevent conflicts
#define EEPROM_ADDRESS_INITIAL_TIME 0
#define EEPROM_ADDRESS_WIFI_CREDENTIALS 4  // After initial time (4 bytes)
#define EEPROM_ADDRESS_SYSTEM_STATE 100    // System configuration

// ============================================================================
// HARDWARE CONSTANTS
// ============================================================================
// Power management
#define POWER_PIN 2  // Power-off detection pin

// Stepper and control pins
#define STEP_PIN 8
#define DIR_PIN 7
#define ENABLE_PIN 3
#define MS1_PIN 4
#define MS2_PIN 5
#define MS3_PIN 6
#define LED_PIN 13

// ============================================================================
// TIMING CONSTANTS
// ============================================================================
// State machine timeouts
#define WIFI_CONNECT_TIMEOUT 30000UL      // 30 seconds
#define NTP_SYNC_TIMEOUT 30000UL          // 30 seconds
#define CONFIG_TIMEOUT 300000UL           // 5 minutes
#define ERROR_RECOVERY_DELAY 5000UL       // 5 seconds

// ============================================================================
// NETWORK CONSTANTS
// ============================================================================
#define NTP_SYNC_INTERVAL 3600000UL       // 1 hour (in milliseconds)
#define AP_SSID "MechanicalClock"         // Access point SSID
#define AP_PASSWORD "12345678"            // Access point password

#endif // CONSTANTS_H 
#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============================================================================
// EEPROM ADDRESSES
// ============================================================================
// Centralized EEPROM address definitions to prevent conflicts
#define EEPROM_ADDRESS_INITIAL_TIME 0
#define EEPROM_ADDRESS_WIFI_CREDENTIALS 4  // After initial time (4 bytes)
#define EEPROM_ADDRESS_SYSTEM_STATE 100    // System configuration

// EEPROM Addresses for Power Recovery
#define EEPROM_ADDRESS_POWER_STATE 8         // Power-down state information
#define EEPROM_ADDRESS_RECOVERY_FLAG 16      // Recovery validation flag
#define EEPROM_ADDRESS_TEST_MODE 24          // Test mode flag for simulation

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

// Power Recovery Constants
#define POWER_RECOVERY_TIMEOUT 30000UL    // 30 seconds timeout for recovery
#define MIN_VALID_POWER_DOWN_TIME 1672531200UL // Jan 1, 2023 - minimum valid timestamp
#define MAX_VALID_POWER_DOWN_TIME 4102444800UL // Jan 1, 2100 - maximum valid timestamp

// Power Recovery States
#define POWER_STATE_RUNNING 0x01         // System was running normally
#define POWER_STATE_ERROR 0x02           // System was in error state
#define POWER_STATE_CONFIG 0x03          // System was in configuration
#define POWER_STATE_TEST 0xFF            // Test mode simulation

// Recovery Validation
#define RECOVERY_VALIDATION_MAGIC 0xDEADBEEF // Magic number for validation

// ============================================================================
// NETWORK CONSTANTS
// ============================================================================
#define NTP_SYNC_INTERVAL 3600000UL       // 1 hour (in milliseconds)
#define AP_SSID "MechanicalClock"         // Access point SSID
#define AP_PASSWORD "12345678"            // Access point password

#endif // CONSTANTS_H 
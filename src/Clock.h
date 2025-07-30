#ifndef CLOCK_H
#define CLOCK_H

#include <RTC.h> // For RTCTime and RTClock
#include <EEPROM.h> // For EEPROM.put()
#include "LCDDisplay.h" // LCDDisplay will be a dependency
#include "Constants.h" // For EEPROM_ADDRESS_INITIAL_TIME

class Clock {
protected:
    RTClock& _rtc; 
    LCDDisplay& _lcd;

public:
    Clock(RTClock& rtcRef, LCDDisplay& lcdRef) : _rtc(rtcRef), _lcd(lcdRef) {}

    virtual void begin() = 0; // For any initial setup specific to the clock type
    virtual void updateCurrentTime() = 0; // Unified time update method (normal operation + sync events)
    virtual void handlePowerOff(); // Common power-off handling (ISR safe) - saves current time to EEPROM
    
    // Enhanced power recovery methods
    bool simulatePowerOff(uint8_t state = POWER_STATE_RUNNING); // Test method to simulate power-off
    bool validatePowerRecoveryData(); // Validate saved power recovery data
    void clearPowerRecoveryData(); // Clear all power recovery data
    time_t getPowerDownTime(); // Get saved power-down time
    uint8_t getPowerDownState(); // Get saved power-down state
    bool isTestMode(); // Check if in test mode
};

// Implementation of common power-off handling
inline void Clock::handlePowerOff() {
    // Save current time to EEPROM for power recovery
    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 
    time_t timeToSave = currentRTCtime.getUnixTime();
    
    // Save time, state, and validation flag
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);
    EEPROM.put(EEPROM_ADDRESS_POWER_STATE, POWER_STATE_RUNNING);
    EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, RECOVERY_VALIDATION_MAGIC);
    
    Serial.println("Power-off data saved to EEPROM");
}

// Test method to simulate power-off without actual power loss
inline bool Clock::simulatePowerOff(uint8_t state) {
    Serial.println("=== SIMULATING POWER-OFF ===");
    
    // Save current time and state
    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 
    time_t timeToSave = currentRTCtime.getUnixTime();
    
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);
    EEPROM.put(EEPROM_ADDRESS_POWER_STATE, state);
    EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, RECOVERY_VALIDATION_MAGIC);
    EEPROM.put(EEPROM_ADDRESS_TEST_MODE, POWER_STATE_TEST);
    
    Serial.print("Simulated power-off at: "); Serial.println(timeToSave);
    Serial.print("State: "); Serial.println(state);
    Serial.println("=== POWER-OFF SIMULATION COMPLETE ===");
    
    return true;
}

// Validate saved power recovery data
inline bool Clock::validatePowerRecoveryData() {
    time_t savedTime;
    uint8_t savedState;
    uint32_t validationFlag;
    uint8_t testMode;
    
    EEPROM.get(EEPROM_ADDRESS_INITIAL_TIME, savedTime);
    EEPROM.get(EEPROM_ADDRESS_POWER_STATE, savedState);
    EEPROM.get(EEPROM_ADDRESS_RECOVERY_FLAG, validationFlag);
    EEPROM.get(EEPROM_ADDRESS_TEST_MODE, testMode);
    
    // Check validation magic number
    if (validationFlag != RECOVERY_VALIDATION_MAGIC) {
        Serial.println("Power recovery data validation failed - invalid magic number");
        return false;
    }
    
    // Check time validity
    if (savedTime < MIN_VALID_POWER_DOWN_TIME || savedTime > MAX_VALID_POWER_DOWN_TIME) {
        Serial.println("Power recovery data validation failed - invalid timestamp");
        return false;
    }
    
    // Check state validity
    if (savedState != POWER_STATE_RUNNING && savedState != POWER_STATE_ERROR && 
        savedState != POWER_STATE_CONFIG && savedState != POWER_STATE_TEST) {
        Serial.println("Power recovery data validation failed - invalid state");
        return false;
    }
    
    Serial.println("Power recovery data validation passed");
    return true;
}

// Clear all power recovery data
inline void Clock::clearPowerRecoveryData() {
    time_t clearTime = 0;
    uint8_t clearState = 0;
    uint32_t clearFlag = 0;
    uint8_t clearTest = 0;
    
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, clearTime);
    EEPROM.put(EEPROM_ADDRESS_POWER_STATE, clearState);
    EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, clearFlag);
    EEPROM.put(EEPROM_ADDRESS_TEST_MODE, clearTest);
    
    Serial.println("Power recovery data cleared");
}

// Get saved power-down time
inline time_t Clock::getPowerDownTime() {
    time_t savedTime;
    EEPROM.get(EEPROM_ADDRESS_INITIAL_TIME, savedTime);
    return savedTime;
}

// Get saved power-down state
inline uint8_t Clock::getPowerDownState() {
    uint8_t savedState;
    EEPROM.get(EEPROM_ADDRESS_POWER_STATE, savedState);
    return savedState;
}

// Check if in test mode
inline bool Clock::isTestMode() {
    uint8_t testMode;
    EEPROM.get(EEPROM_ADDRESS_TEST_MODE, testMode);
    return (testMode == POWER_STATE_TEST);
}

#endif // CLOCK_H 
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
    virtual void update() = 0;   // The main operational loop logic for the clock type
    virtual void updateCurrentTime() = 0; // Sync to current time (unified for all scenarios)
    virtual void handlePowerOff(); // Common power-off handling (ISR safe) - saves current time to EEPROM
};

// Implementation of common power-off handling
inline void Clock::handlePowerOff() {
    // Save current time to EEPROM for power recovery
    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 
    time_t timeToSave = currentRTCtime.getUnixTime();
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);
}

#endif // CLOCK_H 
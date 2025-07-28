#ifndef CLOCK_H
#define CLOCK_H

#include <RTC.h> // For RTCTime and RTClock
#include "LCDDisplay.h" // LCDDisplay will be a dependency

class Clock {
protected:
    RTClock& _rtc; 
    LCDDisplay& _lcd;

public:
    Clock(RTClock& rtcRef, LCDDisplay& lcdRef) : _rtc(rtcRef), _lcd(lcdRef) {}

    virtual void begin() = 0; // For any initial setup specific to the clock type
    virtual void update() = 0;   // The main operational loop logic for the clock type
    virtual void updateCurrentTime() = 0; // Sync to current time (unified for all scenarios)
    virtual void handlePowerOff() = 0; // Power-off handling (ISR safe)
};

#endif // CLOCK_H 
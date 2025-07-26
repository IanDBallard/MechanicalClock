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
    virtual void update() = 0;   // Renamed: The main operational loop logic for the clock type

    virtual void adjustToInitialTime(time_t initialUnixTime) = 0; 
    virtual void handlePowerOff() = 0;
};

#endif // CLOCK_H 
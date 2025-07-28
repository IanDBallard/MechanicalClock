#ifndef DIGITAL_CLOCK_H
#define DIGITAL_CLOCK_H

#include "Clock.h"      // Inherits from Clock
#include "LCDDisplay.h" // Dependency for display updates

class DigitalClock : public Clock {
private:
    int _lastDisplayedSecond;
    int _lastDisplayedMinute;
    int _lastDisplayedHour;
    int _lastDisplayedDay;
    int _lastDisplayedMonth;
    int _lastDisplayedYear;

public:
    DigitalClock(RTClock& rtcRef, LCDDisplay& lcdRef);

    void begin() override;
    void update() override;   // Optimized updates - only when values change
    void updateCurrentTime() override; // Force update for time sync events
    // handlePowerOff() inherited from base class - no override needed
};

#endif // DIGITAL_CLOCK_H 
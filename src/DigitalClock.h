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

    // Helper methods to eliminate code duplication
    void updateTrackingVariables(const RTCTime& currentTime);
    void forceDisplayUpdate(const RTCTime& currentTime);

public:
    DigitalClock(RTClock& rtcRef, LCDDisplay& lcdRef);

    void begin() override;
    void updateCurrentTime() override; // Unified time update method (optimized + forced updates)
    // handlePowerOff() inherited from base class - no override needed
};

#endif // DIGITAL_CLOCK_H 
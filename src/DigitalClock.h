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

    // bool calculateDST(const RTCTime& time); // This should be in TimeUtils

public:
    DigitalClock(RTClock& rtcRef, LCDDisplay& lcdRef);

    void begin() override;
    void update() override;   // Renamed from run()

        void adjustToInitialTime(time_t initialUnixTime) override;
    void handlePowerOff() override;
    void updateCurrentTime() override; // Sync to current time (unified display update logic)
};

#endif // DIGITAL_CLOCK_H 
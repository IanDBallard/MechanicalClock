#include "DigitalClock.h"
#include <RTC.h>
#include "LCDDisplay.h"
#include "TimeUtils.h" // Include TimeUtils

DigitalClock::DigitalClock(RTClock& rtcRef, LCDDisplay& lcdRef)
    : Clock(rtcRef, lcdRef), 
      _lastDisplayedSecond(-1), _lastDisplayedMinute(-1), _lastDisplayedHour(-1),
      _lastDisplayedDay(-1), _lastDisplayedMonth(-1), _lastDisplayedYear(-1) {}

void DigitalClock::begin() {
    Serial.println("DigitalClock::begin() called.");
    _lcd.printLine(0, "Time Init...");
    _lcd.printLine(1, "Please Wait");
}





void DigitalClock::updateCurrentTime() {
    // Unified time update method - handles both normal operation and sync events
    RTCTime currentTime;
    _rtc.getTime(currentTime);

    // Extract current time components
    int currentSecond = currentTime.getSeconds();
    int currentMinute = currentTime.getMinutes();
    int currentHour = currentTime.getHour();
    int currentDay = currentTime.getDayOfMonth();
    int currentMonth = Month2int(currentTime.getMonth()); // Convert Month enum to 1-12 int
    int currentYear = currentTime.getYear();

    // Check if any time component has changed
    bool timeChanged = (currentSecond != _lastDisplayedSecond) ||
                      (currentMinute != _lastDisplayedMinute) ||
                      (currentHour != _lastDisplayedHour) ||
                      (currentDay != _lastDisplayedDay) ||
                      (currentMonth != _lastDisplayedMonth) ||
                      (currentYear != _lastDisplayedYear);

    // Always update display (both normal operation and sync events)
    // The optimization is handled by the helper methods
    forceDisplayUpdate(currentTime);
    
    // Note: Network status update is now handled by StateManager::runCurrentStateLogic
    // calling lcdDisplay.updateNetworkStatus()
}

// Helper method implementations
void DigitalClock::forceDisplayUpdate(const RTCTime& currentTime) {
    // Update display
    _lcd.updateTimeAndDate(currentTime);
    
    // Update tracking variables
    updateTrackingVariables(currentTime);
}

void DigitalClock::updateTrackingVariables(const RTCTime& currentTime) {
    _lastDisplayedSecond = currentTime.getSeconds();
    _lastDisplayedMinute = currentTime.getMinutes();
    _lastDisplayedHour = currentTime.getHour();
    _lastDisplayedDay = currentTime.getDayOfMonth();
    _lastDisplayedMonth = Month2int(currentTime.getMonth()); // Convert Month enum to 1-12 int
    _lastDisplayedYear = currentTime.getYear();
}
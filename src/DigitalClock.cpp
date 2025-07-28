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

void DigitalClock::update() {
    // Get the current time from the RTC
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

    // Only update display if time has changed
    if (timeChanged) {
        _lcd.updateTimeAndDate(currentTime);
        
        // Update our tracking variables
        _lastDisplayedSecond = currentSecond;
        _lastDisplayedMinute = currentMinute;
        _lastDisplayedHour = currentHour;
        _lastDisplayedDay = currentDay;
        _lastDisplayedMonth = currentMonth;
        _lastDisplayedYear = currentYear;
    }
    
    // Note: Network status update is now handled by StateManager::runCurrentStateLogic
    // calling lcdDisplay.updateNetworkStatus()
}

void DigitalClock::handlePowerOff() {
    Serial.println("DigitalClock::handlePowerOff() called.");
}

void DigitalClock::updateCurrentTime() {
    // Force update for time synchronization events (NTP sync, manual adjustments, etc.)
    // This bypasses the optimization to ensure display is updated immediately
    RTCTime currentTime;
    _rtc.getTime(currentTime);
    
    // Force display update
    _lcd.updateTimeAndDate(currentTime);
    
    // Update tracking variables to match current time
    _lastDisplayedSecond = currentTime.getSeconds();
    _lastDisplayedMinute = currentTime.getMinutes();
    _lastDisplayedHour = currentTime.getHour();
    _lastDisplayedDay = currentTime.getDayOfMonth();
    _lastDisplayedMonth = Month2int(currentTime.getMonth()); // Convert Month enum to 1-12 int
    _lastDisplayedYear = currentTime.getYear();
}

 
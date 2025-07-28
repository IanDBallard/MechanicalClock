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

void DigitalClock::update() { // Renamed from run()
    // Get the current time from the RTC
    RTCTime currentTime;
    _rtc.getTime(currentTime); 

    // Delegate time and date display update to the LCDDisplay object
    _lcd.updateTimeAndDate(currentTime);
    
    // Note: Network status update is now handled by StateManager::runCurrentStateLogic
    // calling lcdDisplay.updateNetworkStatus()
}



void DigitalClock::handlePowerOff() {
    Serial.println("DigitalClock::handlePowerOff() called.");
}

void DigitalClock::updateCurrentTime() {
    // Unified display update logic for all time synchronization events
    // (NTP sync, RTC updates, manual adjustments, etc.)
    RTCTime currentTime;
    _rtc.getTime(currentTime);
    
    // Update display with current time
    _lcd.updateTimeAndDate(currentTime);
}

 
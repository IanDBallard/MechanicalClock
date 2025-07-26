#include "MechanicalClock.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <RTC.h>
#include "LCDDisplay.h" // Needed for LCDDisplay methods (though not directly called in update())
#include "TimeUtils.h"  // For TimeUtils functions

MechanicalClock::MechanicalClock(int stepPin, int dirPin, int enablePin, int ms1Pin, int ms2Pin, int ms3Pin, int ledPin,
                                 RTClock& rtcRef, LCDDisplay& lcdRef)
    : Clock(rtcRef, lcdRef),
      _myStepper(AccelStepper::DRIVER, stepPin, dirPin),
      _activityLED(ledPin),
      _enablePin(enablePin), _ms1Pin(ms1Pin), _ms2Pin(ms2Pin), _ms3Pin(ms3Pin),
      _stepperIdleTimeout(5000),
      _currentClockTime(0),
      _lastStepperMoveTime(0)
{
    _stepsPerRevolution = 0;
    _secondsPerStep = 0;
}

void MechanicalClock::_enableStepperDriver() {
    digitalWrite(_enablePin, LOW); // LOW enables A4988 driver
}

void MechanicalClock::_disableStepperDriver() {
    digitalWrite(_enablePin, HIGH); // HIGH disables A4988 driver
}

void MechanicalClock::_setMicrostepping(uint8_t mode) {
    digitalWrite(_ms1Pin, (mode & 0b100) ? HIGH : LOW);
    digitalWrite(_ms2Pin, (mode & 0b010) ? HIGH : LOW);
    digitalWrite(_ms3Pin, (mode & 0b001) ? HIGH : LOW);
    
    // Calculate steps per revolution based on microstepping
    int microstepMultiplier = 1;
    switch (mode) {
        case MICROSTEP_FULL: microstepMultiplier = 1; break;
        case MICROSTEP_HALF: microstepMultiplier = 2; break;
        case MICROSTEP_QUARTER: microstepMultiplier = 4; break;
        case MICROSTEP_EIGHTH: microstepMultiplier = 8; break;
        case MICROSTEP_SIXTEENTH: microstepMultiplier = 16; break;
        default: microstepMultiplier = 1; break;
    }
    
    _stepsPerRevolution = BASE_STEPS_PER_REV * microstepMultiplier;
    _secondsPerStep = 18 / microstepMultiplier; // 18 seconds per step for full stepping
}

void MechanicalClock::begin() {
    Serial.println("MechanicalClock::begin() called.");
    pinMode(_enablePin, OUTPUT);
    _disableStepperDriver();

    setMicrosteppingMode(CURRENT_MICROSTEP);

    _myStepper.setMaxSpeed(50);
    _myStepper.setAcceleration(2);
    _myStepper.setSpeed(5);

    _activityLED.on();
    delay(200);
    _activityLED.off();

    Serial.println("MechanicalClock initialized.");
}

void MechanicalClock::update() { // Renamed from run()
    _myStepper.run();

    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 

    long rawTimeDiff = currentRTCtime.getUnixTime() - _currentClockTime;

    long effectiveTimeDiffForSteps = rawTimeDiff;
    if (effectiveTimeDiffForSteps > SECONDS_IN_12_HOURS / 2) { 
        effectiveTimeDiffForSteps -= SECONDS_IN_12_HOURS;
    } else if (effectiveTimeDiffForSteps <= -SECONDS_IN_12_HOURS / 2) {
        effectiveTimeDiffForSteps += SECONDS_IN_12_HOURS;
    }

    long stepsNeeded = effectiveTimeDiffForSteps / _secondsPerStep; 

    if (stepsNeeded != 0) {
        if (abs(stepsNeeded) > 1) { 
             Serial.print("[DEBUG] Targeting Move! Current DistToGo: "); Serial.print(_myStepper.distanceToGo());
             Serial.print(", Additional StepsNeeded: "); Serial.print(stepsNeeded);
             Serial.print(", RawDiff: "); Serial.print(rawTimeDiff);
             Serial.print(", EffDiff: "); Serial.println(effectiveTimeDiffForSteps);
        }
        _myStepper.move(_myStepper.distanceToGo() + stepsNeeded); 
        _currentClockTime += stepsNeeded * _secondsPerStep;
        if (_currentClockTime != (currentRTCtime.getUnixTime() / _secondsPerStep) * _secondsPerStep) { // More robust check against actual RTC time
             Serial.print("[DEBUG] _currentClockTime updated to "); Serial.print(_currentClockTime);
             Serial.print(" from RTC: "); Serial.println(currentRTCtime.getUnixTime());
        }
    }

    if (_myStepper.distanceToGo() == 0) {
        _activityLED.off();
        if (millis() - _lastStepperMoveTime > _stepperIdleTimeout) {
            _disableStepperDriver(); 
        }
    } else {
        _enableStepperDriver();
        _activityLED.on();
        _lastStepperMoveTime = millis();
    }
}

void MechanicalClock::adjustToInitialTime(time_t initialUnixTime) {
    Serial.print("MechanicalClock::adjustToInitialTime() called with: ");
    Serial.println(initialUnixTime);

    _currentClockTime = initialUnixTime; 

    long targetPositionSteps = initialUnixTime / _secondsPerStep;
    _myStepper.moveTo(targetPositionSteps);
    Serial.print("Adjusting to absolute position: "); Serial.println(targetPositionSteps);
    Serial.print("Current stepper position after moveTo: "); Serial.println(_myStepper.currentPosition());

    _enableStepperDriver(); 
    _lastStepperMoveTime = millis();
}

void MechanicalClock::handlePowerOff() {
    Serial.println("MechanicalClock::handlePowerOff() ISR called.");
    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 

    time_t timeToSave = currentRTCtime.getUnixTime();
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);

    _activityLED.on(); 
    digitalWrite(_enablePin, HIGH); 
}

void MechanicalClock::setMicrosteppingMode(uint8_t mode) {
    _setMicrostepping(mode);
}

// _calculateStepsToAlign is kept as a protected helper, but not directly used in new run() logic.
int MechanicalClock::_calculateStepsToAlign(int prevHour, int prevMinute, int currentHour, int currentMinute) {
    int elapsedMinutes = (currentHour * 60 + currentMinute) - (prevHour * 60 + prevMinute);
    if (elapsedMinutes < 0) {
        elapsedMinutes += 24 * 60;
    }
    int minuteSteps = (long)elapsedMinutes * _stepsPerRevolution / 60;
    return minuteSteps;
} 
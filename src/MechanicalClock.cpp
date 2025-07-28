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
    
    // Hardware initialization
    _activityLED.begin(); // Initialize LED pin
    pinMode(_enablePin, OUTPUT);
    _disableStepperDriver();
    setMicrosteppingMode(CURRENT_MICROSTEP);
    _myStepper.setMaxSpeed(50);
    _myStepper.setAcceleration(2);
    _myStepper.setSpeed(5);
    
    // Power recovery logic
    time_t powerDownTime = 0;
    EEPROM.get(EEPROM_ADDRESS_INITIAL_TIME, powerDownTime);
    Serial.print("Power-down time from EEPROM: "); Serial.println(powerDownTime);
    
    if (powerDownTime != 0) {
        Serial.println("Power-down time found - will calculate stepper adjustment");
        // Clear the saved time immediately to avoid re-using it
        time_t clearValue = 0;
        EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, clearValue);
        Serial.println("✓ Cleared saved power-down time from EEPROM.");
        
        // Calculate and set initial position based on power-down time
        adjustToInitialTime(powerDownTime);
    } else {
        Serial.println("No power-down time found - assuming warm boot, sync to current time");
        // Sync to current time using unified updateCurrentTime method
        updateCurrentTime();
    }
    
    _activityLED.on();
    delay(200);
    _activityLED.off();
    
    Serial.println("MechanicalClock initialized.");
}

void MechanicalClock::update() {
    _myStepper.run();

    // Get current UTC time from RTC
    time_t currentUTC = getCurrentUTC();

    // Calculate time difference since last update
    long timeDiff = currentUTC - _currentClockTime;
    
    // Only move if we have accumulated enough time for at least one step
    if (abs(timeDiff) >= _secondsPerStep) {
        long stepsNeeded = timeDiff / _secondsPerStep;
        
        // Limit to reasonable movement (sanity check)
        if (abs(stepsNeeded) > 100) {
            Serial.print("[WARNING] Excessive steps detected: "); Serial.print(stepsNeeded);
            Serial.print(" (TimeDiff: "); Serial.print(timeDiff);
            Serial.println(") - Limiting to reasonable value");
            stepsNeeded = (stepsNeeded > 0) ? 100 : -100;
        }
        
        if (stepsNeeded != 0) {
            if (abs(stepsNeeded) > 1) { 
                Serial.print("[DEBUG] Normal movement - StepsNeeded: "); Serial.print(stepsNeeded);
                Serial.print(", TimeDiff: "); Serial.println(timeDiff);
            }
            
            _myStepper.move(_myStepper.distanceToGo() + stepsNeeded); 
            _currentClockTime += stepsNeeded * _secondsPerStep;
        }
    }

    // Handle stepper driver enable/disable and LED
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
    Serial.print("adjustToInitialTime called with: "); Serial.println(initialUnixTime);
    
    // Get current UTC time
    time_t currentUTC = getCurrentUTC();
    Serial.print("Current UTC time: "); Serial.println(currentUTC);
    
    // Set clock time to current time (not the initial time)
    _currentClockTime = currentUTC;
    
    if (initialUnixTime == 0) {
        // No power-down time - set position to current time, no movement needed
        Serial.println("No power-down time - setting position to current time");
        long secondsIn12HourCycle = currentUTC % SECONDS_IN_12_HOURS;
        long targetPositionSteps = secondsIn12HourCycle / _secondsPerStep;
        
        Serial.print("Target position steps: "); Serial.println(targetPositionSteps);
        _myStepper.setCurrentPosition(targetPositionSteps);
    } else {
        // Power-down time exists - calculate movement needed
        Serial.println("Power-down time exists - calculating movement");
        
        // Calculate time difference
        long timeDiff = currentUTC - initialUnixTime;
        Serial.print("Time difference: "); Serial.println(timeDiff);
        
        // Get positions within 12-hour cycle
        long powerDownPosition = initialUnixTime % SECONDS_IN_12_HOURS;
        long currentPosition = currentUTC % SECONDS_IN_12_HOURS;
        
        Serial.print("Power-down position in cycle: "); Serial.println(powerDownPosition);
        Serial.print("Current position in cycle: "); Serial.println(currentPosition);
        
        // Calculate shortest path distance
        long distance = currentPosition - powerDownPosition;
        
        // Handle 12-hour cycle wrap-around for shortest path
        if (distance > SECONDS_IN_12_HOURS / 2) {
            distance -= SECONDS_IN_12_HOURS;
        } else if (distance <= -SECONDS_IN_12_HOURS / 2) {
            distance += SECONDS_IN_12_HOURS;
        }
        
        Serial.print("Shortest path distance: "); Serial.println(distance);
        
        // Convert to steps
        long stepsNeeded = distance / _secondsPerStep;
        Serial.print("Steps needed: "); Serial.println(stepsNeeded);
        
        // Set current position to power-down position, then move to current position
        long powerDownSteps = powerDownPosition / _secondsPerStep;
        _myStepper.setCurrentPosition(powerDownSteps);
        
        Serial.print("Set stepper position to: "); Serial.println(powerDownSteps);
        
        if (stepsNeeded != 0) {
            long targetPosition = powerDownSteps + stepsNeeded;
            Serial.print("Moving to target position: "); Serial.println(targetPosition);
            _myStepper.moveTo(targetPosition);
        }
    }

    _enableStepperDriver(); 
    _lastStepperMoveTime = millis();
    Serial.println("adjustToInitialTime complete");
}

void MechanicalClock::handlePowerOff() {
    RTCTime currentRTCtime;
    _rtc.getTime(currentRTCtime); 

    time_t timeToSave = currentRTCtime.getUnixTime();
    EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);

    _activityLED.on(); 
    digitalWrite(_enablePin, HIGH); 
}

void MechanicalClock::updateCurrentTime() {
    // Unified stepper movement logic for all time synchronization events
    // (NTP sync, startup, manual adjustments, etc.)
    time_t currentUTC = getCurrentUTC();
    long timeDiff = currentUTC - _currentClockTime;
    
    if (abs(timeDiff) >= _secondsPerStep) {
        long stepsNeeded = timeDiff / _secondsPerStep;
        
        // Limit to reasonable movement (sanity check)
        if (abs(stepsNeeded) > 100) {
            Serial.print("[WARNING] Excessive steps detected: "); Serial.print(stepsNeeded);
            Serial.print(" (TimeDiff: "); Serial.print(timeDiff);
            Serial.println(") - Limiting to reasonable value");
            stepsNeeded = (stepsNeeded > 0) ? 100 : -100;
        }
        
        if (stepsNeeded != 0) {
            Serial.print("[DEBUG] updateCurrentTime - StepsNeeded: "); Serial.print(stepsNeeded);
            Serial.print(", TimeDiff: "); Serial.println(timeDiff);
            
            _myStepper.move(_myStepper.distanceToGo() + stepsNeeded); 
            _currentClockTime += stepsNeeded * _secondsPerStep;
        }
    }
}

void MechanicalClock::setMicrosteppingMode(uint8_t mode) {
    _setMicrostepping(mode);
}



 
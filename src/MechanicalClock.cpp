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
        
        // Set initial time for power recovery calculation, then sync
        _currentClockTime = powerDownTime;
        updateCurrentTime(); // Will detect large movement and use shortest path
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





void MechanicalClock::handlePowerOff() {
    // Call base class to save current time to EEPROM
    Clock::handlePowerOff();
    
    // Mechanical-specific power-off handling
    _activityLED.on(); 
    digitalWrite(_enablePin, HIGH); // Disable stepper driver
}

void MechanicalClock::updateCurrentTime() {
    // Unified time update method - handles both normal operation and sync events
    _myStepper.run(); // Always run stepper to execute any pending movements

    // Get current UTC time from RTC
    time_t currentUTC = getCurrentUTC();
    long timeDiff = currentUTC - _currentClockTime;
    
    // If time difference is large (> 6 hours), use shortest path logic
    if (abs(timeDiff) > SECONDS_IN_12_HOURS / 2) {
        Serial.println("[DEBUG] Large time difference detected - using shortest path calculation");
        
        // Large movement - use shortest path calculation
        long currentPosition = _currentClockTime % SECONDS_IN_12_HOURS;
        long targetPosition = currentUTC % SECONDS_IN_12_HOURS;
        long distance = targetPosition - currentPosition;
        
        Serial.print("[DEBUG] Current position in cycle: "); Serial.println(currentPosition);
        Serial.print("[DEBUG] Target position in cycle: "); Serial.println(targetPosition);
        Serial.print("[DEBUG] Raw distance: "); Serial.println(distance);
        
        // Handle 12-hour cycle wrap-around for shortest path
        if (distance > SECONDS_IN_12_HOURS / 2) {
            distance -= SECONDS_IN_12_HOURS;
        } else if (distance <= -SECONDS_IN_12_HOURS / 2) {
            distance += SECONDS_IN_12_HOURS;
        }
        
        Serial.print("[DEBUG] Shortest path distance: "); Serial.println(distance);
        
        long stepsNeeded = distance / _secondsPerStep;
        Serial.print("[DEBUG] Steps needed (shortest path): "); Serial.println(stepsNeeded);
        
        if (stepsNeeded != 0) {
            _myStepper.move(_myStepper.distanceToGo() + stepsNeeded);
            _currentClockTime = currentUTC; // Set to exact target time
        }
    } else {
        // Normal movement - use standard logic
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

void MechanicalClock::setMicrosteppingMode(uint8_t mode) {
    _setMicrostepping(mode);
}



 
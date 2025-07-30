#include "MechanicalClock.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <RTC.h>
#include "LCDDisplay.h" // Needed for LCDDisplay methods (though not directly called in update())
#include "TimeUtils.h"  // For TimeUtils functions

// Helper function to format time for debugging
String formatTime(time_t unixTime) {
    struct tm* timeinfo = localtime(&unixTime);
    char buffer[20];
    sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return String(buffer);
}

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
    // Initialize with proper values immediately
    _setMicrostepping(CURRENT_MICROSTEP);
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
    // TODO: Consider implementing fractional accumulator to handle non-integer division results
    // For microstepping modes > 2, this integer division loses precision (e.g., 18/4=4 instead of 4.5)
    // Solution: Accumulate fractional error and add catch-up steps when error >= 1.0
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
    
    // Enhanced power recovery logic
    Serial.println("=== POWER RECOVERY ANALYSIS ===");
    
    // Check if we have valid power recovery data
    if (validatePowerRecoveryData()) {
        time_t powerDownTime = getPowerDownTime();
        uint8_t powerDownState = getPowerDownState();
        bool testMode = isTestMode();
        
        Serial.print("Power-down time from EEPROM: "); Serial.println(powerDownTime);
        Serial.print("Power-down state: "); Serial.println(powerDownState);
        Serial.print("Test mode: "); Serial.println(testMode ? "YES" : "NO");
        
        if (powerDownTime != 0) {
            Serial.println("✓ Valid power-down time found - will calculate stepper adjustment after NTP sync");
            
            // Clear the saved data immediately to avoid re-using it
            clearPowerRecoveryData();
            Serial.println("✓ Cleared saved power recovery data from EEPROM.");
            
            // Store the power-down time for use after NTP sync
            _currentClockTime = powerDownTime;
            
            // If this was a test simulation, provide immediate feedback
            if (testMode) {
                Serial.println("=== TEST MODE DETECTED ===");
                Serial.println("Power recovery simulation successful!");
                Serial.println("Clock will adjust position after NTP sync.");
            }
        } else {
            Serial.println("No power-down time found - will wait for NTP sync before calculating stepper position");
            _currentClockTime = 0;
        }
    } else {
        Serial.println("No valid power recovery data found - starting fresh");
        _currentClockTime = 0;
    }
    
    Serial.println("=== POWER RECOVERY ANALYSIS COMPLETE ===");
    
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
    
    // If _currentClockTime is 0, this is the first sync after startup
    // Just set the current time without calculating movement
    if (_currentClockTime == 0) {
        Serial.println("[DEBUG] First time sync - setting current position without movement");
        _currentClockTime = currentUTC;
        return;
    }
    
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
            // DETAILED DEBUGGING FOR ANTICLOCKWISE MOVEMENTS
            if (stepsNeeded < 0) {
                Serial.println("*** ANTICLOCKWISE MOVEMENT DETECTED (Large Time Diff) ***");
                Serial.print("Net Movement: "); Serial.print(stepsNeeded); Serial.println(" steps");
                Serial.print("Distance in seconds: "); Serial.println(distance);
                Serial.print("Current Clock Time: "); Serial.print(_currentClockTime);
                Serial.print(" ("); Serial.print(formatTime(_currentClockTime)); Serial.println(")");
                Serial.print("UTC Real Time: "); Serial.print(currentUTC);
                Serial.print(" ("); Serial.print(formatTime(currentUTC)); Serial.println(")");
                Serial.print("Time Difference: "); Serial.print(timeDiff); Serial.println(" seconds");
                Serial.println("*** END ANTICLOCKWISE DEBUG ***");
            }
            
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
                // DETAILED DEBUGGING FOR ANTICLOCKWISE MOVEMENTS
                if (stepsNeeded < 0) {
                    Serial.println("*** ANTICLOCKWISE MOVEMENT DETECTED (Normal) ***");
                    Serial.print("Net Movement: "); Serial.print(stepsNeeded); Serial.println(" steps");
                    Serial.print("Time Difference: "); Serial.print(timeDiff); Serial.println(" seconds");
                    Serial.print("Current Clock Time: "); Serial.print(_currentClockTime);
                    Serial.print(" ("); Serial.print(formatTime(_currentClockTime)); Serial.println(")");
                    Serial.print("UTC Real Time: "); Serial.print(currentUTC);
                    Serial.print(" ("); Serial.print(formatTime(currentUTC)); Serial.println(")");
                    Serial.println("*** END ANTICLOCKWISE DEBUG ***");
                } else if (abs(stepsNeeded) > 1) { 
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



 
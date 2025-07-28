#ifndef MECHANICAL_CLOCK_H
#define MECHANICAL_CLOCK_H

#include "Clock.h"        // Base class
#include <AccelStepper.h> // For stepper motor control
#include <EEPROM.h>       // For saving/loading initial time
#include "LED.h"          // Include LED class
#include "Constants.h"    // Centralized constants

// Microstepping constants
#define MICROSTEP_FULL 0b000
#define MICROSTEP_HALF 0b100
#define MICROSTEP_QUARTER 0b010
#define MICROSTEP_EIGHTH 0b110
#define MICROSTEP_SIXTEENTH 0b111

// Current microstepping mode
#define CURRENT_MICROSTEP MICROSTEP_FULL

// Base steps per revolution (for full stepping)
#define BASE_STEPS_PER_REV 200

// 12-hour cycle in seconds
#define SECONDS_IN_12_HOURS 43200

class MechanicalClock : public Clock {
private:
    AccelStepper _myStepper;
    LED _activityLED; 

    const int _enablePin;
    const int _ms1Pin;
    const int _ms2Pin;
    const int _ms3Pin;

    int _stepsPerRevolution;
    int _secondsPerStep; 

    time_t _currentClockTime; // Unix timestamp the hands represent
    
    unsigned long _lastStepperMoveTime;
    const unsigned long _stepperIdleTimeout;
    


    void _setMicrostepping(uint8_t mode);
    void _enableStepperDriver();
    void _disableStepperDriver();

public:
    MechanicalClock(int stepPin, int dirPin, int enablePin, int ms1Pin, int ms2Pin, int ms3Pin, int ledPin,
                    RTClock& rtcRef, LCDDisplay& lcdRef);

    void begin() override;
    void update() override;   // Renamed from run()
    void handlePowerOff() override; // Mechanical-specific power-off handling (stepper driver, LED)
    void updateCurrentTime() override; // Sync to current time (unified stepper movement logic)

    void setMicrosteppingMode(uint8_t mode);


};

#endif // MECHANICAL_CLOCK_H 
#ifndef LED_H
#define LED_H

#include <Arduino.h>

// ============================================================================
// LED CONTROL CLASS
// ============================================================================
class LED {
private:
    byte ledPin;
    bool currentState;

public:
    // Constructor
    LED(byte pin);
    
    // Initialization
    void begin();
    
    // Control methods
    void on();
    void off();
    void toggle();
    void setState(bool state);
    
    // Status methods
    bool isOn() const;
    bool getState() const;
};

#endif // LED_H 
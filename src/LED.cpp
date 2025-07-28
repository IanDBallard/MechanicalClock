#include "LED.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
LED::LED(byte pin) : ledPin(pin), currentState(false) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void LED::begin() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    currentState = false;
}

// ============================================================================
// CONTROL METHODS
// ============================================================================
void LED::on() {
    digitalWrite(ledPin, HIGH);
    currentState = true;
}

void LED::off() {
    digitalWrite(ledPin, LOW);
    currentState = false;
}

void LED::toggle() {
    if (currentState) {
        off();
    } else {
        on();
    }
}

void LED::setState(bool state) {
    if (state) {
        on();
    } else {
        off();
    }
}

// ============================================================================
// STATUS METHODS
// ============================================================================
bool LED::isOn() const {
    return currentState;
}

bool LED::getState() const {
    return currentState;
}

 
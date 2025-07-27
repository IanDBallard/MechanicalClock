#include "LED.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================
LED::LED(byte pin) : ledPin(pin), currentState(false), blinking(false) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void LED::begin() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    currentState = false;
    blinking = false;
}

// ============================================================================
// CONTROL METHODS
// ============================================================================
void LED::on() {
    if (blinking) {
        blinking = false;
    }
    digitalWrite(ledPin, HIGH);
    currentState = true;
}

void LED::off() {
    if (blinking) {
        blinking = false;
    }
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

// ============================================================================
// BLINKING FUNCTIONALITY
// ============================================================================
void LED::blink(unsigned long onTime, unsigned long offTime) {
    blinkOnTime = onTime;
    blinkOffTime = offTime;
    blinking = true;
    blinkState = true;
    lastBlinkChange = millis();
    digitalWrite(ledPin, HIGH);
    currentState = true;
}

void LED::updateBlink() {
    if (!blinking) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long timeSinceChange = currentTime - lastBlinkChange;
    
    if (blinkState) {
        // Currently ON
        if (timeSinceChange >= blinkOnTime) {
            digitalWrite(ledPin, LOW);
            blinkState = false;
            currentState = false;
            lastBlinkChange = currentTime;
        }
    } else {
        // Currently OFF
        if (timeSinceChange >= blinkOffTime) {
            digitalWrite(ledPin, HIGH);
            blinkState = true;
            currentState = true;
            lastBlinkChange = currentTime;
        }
    }
} 
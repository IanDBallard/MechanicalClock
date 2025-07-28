# Design Decisions - Mechanical Clock with Onboard RTC

## Overview

This document outlines the key architectural decisions and design patterns used in the Mechanical Clock project. The system is designed to provide accurate timekeeping using a stepper motor-driven mechanical clock face, with network time synchronization and a robust state machine architecture.

## Core Architecture

### 1. Object-Oriented Design with State Machine Pattern

**Decision**: Refactored from monolithic code to modular OOP design with state machine pattern.

**Rationale**: 
- Improves maintainability and testability
- Separates concerns (network, display, clock mechanics, state management)
- Enables easier debugging and feature additions
- Provides clear state transitions and error handling

**Implementation**:
- `StateManager`: Central state machine orchestrating system behavior
- `NetworkManager`: Handles WiFi connectivity and NTP synchronization
- `MechanicalClock`: Manages stepper motor and clock mechanics
- `LCDDisplay`: Controls I2C LCD display
- `TimeUtils`: Time calculations and timezone/DST handling

### 2. UTC-First Time Management

**Decision**: All internal time operations use UTC, with local time conversion only for external interfaces.

**Rationale**:
- Eliminates timezone confusion in internal calculations
- Simplifies NTP synchronization (NTP provides UTC)
- Prevents DST-related bugs in core timekeeping logic
- Makes the system portable across timezones

**Implementation**:
- RTC stores UTC time
- `getCurrentUTC()` returns UTC from RTC
- `convertUTCToLocal()` converts to local time only for display
- NTP sync stores UTC in RTC, not local time

**Example**:
```cpp
// Internal operations use UTC
time_t currentUTC = getCurrentUTC();
_currentClockTime = currentUTC;

// Display converts to local time
RTCTime localTime = convertUTCToLocal(currentUTC, timeZoneOffset, useDST);
```

### 3. Continuous Time Tracking with Stepper Motor

**Decision**: Use AccelStepper's `distanceToGo()` function for continuous time synchronization.

**Rationale**:
- Provides smooth, continuous movement
- Automatically handles acceleration/deceleration
- Maintains position accuracy over long periods
- Reduces stepper motor wear and noise

**Implementation**:
```cpp
void MechanicalClock::update() {
    time_t currentUTC = getCurrentUTC();
    long timeDiff = currentUTC - _currentClockTime;
    
    if (timeDiff >= _secondsPerStep) {
        long stepsNeeded = timeDiff / _secondsPerStep;
        _myStepper.move(stepsNeeded);
        _currentClockTime += stepsNeeded * _secondsPerStep;
    }
    
    _myStepper.run(); // Continuous movement
}
```

### 4. Power-Off Recovery System

**Decision**: Store power-off time in EEPROM and recover position on restart.

**Rationale**:
- Maintains clock accuracy across power cycles
- Prevents clock hands from resetting to 12:00 on restart
- Calculates shortest path movement to current time
- Handles 12-hour cycle wrap-around correctly

**Implementation**:
```cpp
void MechanicalClock::adjustToInitialTime(time_t initialUnixTime) {
    // Calculate shortest path distance
    long distance = currentPosition - powerDownPosition;
    
    // Handle 12-hour cycle wrap-around
    if (distance > SECONDS_IN_12_HOURS / 2) {
        distance -= SECONDS_IN_12_HOURS;
    } else if (distance <= -SECONDS_IN_12_HOURS / 2) {
        distance += SECONDS_IN_12_HOURS;
    }
    
    // Move to current time
    long stepsNeeded = distance / _secondsPerStep;
    _myStepper.moveTo(targetPosition);
}
```

## Network Architecture

### 5. Dual-Mode WiFi Operation

**Decision**: Support both client mode (normal operation) and access point mode (configuration).

**Rationale**:
- Client mode for normal NTP synchronization
- Access point mode for initial WiFi configuration
- Captive portal for easy setup without external tools
- Automatic fallback to AP mode if client connection fails

**Implementation**:
- `NetworkManager::ensureConnection()`: Client mode with retry logic
- `NetworkManager::setupAccessPoint()`: AP mode with captive portal
- State machine transitions between modes based on connection status

### 6. Robust NTP Synchronization

**Decision**: Multiple NTP retry attempts with exponential backoff.

**Rationale**:
- Handles temporary network issues
- Ensures time accuracy even with poor connectivity
- Periodic re-synchronization to maintain accuracy
- Graceful degradation if NTP fails

**Implementation**:
```cpp
for (int i = 0; i < _maxNtpRetries; i++) {
    // Send NTP request
    if (receiveNTPResponse()) {
        // Apply timezone/DST and store UTC in RTC
        return true;
    }
    delay(_ntpRetryDelay);
}
```

## Display Architecture

### 7. Selective LCD Clearing

**Decision**: Clear only time/date areas while preserving network status icons.

**Rationale**:
- Reduces display flicker
- Maintains network status visibility
- Improves user experience
- Preserves important status information

**Implementation**:
```cpp
void LCDDisplay::clearTimeArea() {
    // Clear only time display area (positions 0-14, line 0)
    // Preserve network status icons (position 15)
}
```

### 8. Status Icon System

**Decision**: Use custom characters for WiFi and sync status indicators.

**Rationale**:
- Compact status display
- Clear visual indicators
- Consistent with limited LCD space
- Easy to understand at a glance

## Testing Architecture

### 9. Custom Unit Testing Framework

**Decision**: Implement custom testing framework rather than using external libraries.

**Rationale**:
- Lightweight and Arduino-compatible
- No external dependencies
- Easy to run on target hardware
- Supports mocking of hardware components

**Implementation**:
- `TestFramework.h`: Core testing infrastructure
- `TestRunner.cpp`: Test execution engine
- Mock classes for RTC, WiFi, and other hardware
- Individual test suites for each component

### 10. Isolated Test Execution

**Decision**: Support running individual test suites via dynamic configuration.

**Rationale**:
- Faster debugging of specific components
- Reduced test execution time
- Easier identification of failing tests
- Support for both automated and manual testing

**Implementation**:
- `run_single_test_pio.sh`: Dynamic test environment configuration
- PlatformIO environment filtering
- Temporary test runner generation

## Error Handling and Recovery

### 11. Graceful Degradation

**Decision**: System continues operating with reduced functionality when components fail.

**Rationale**:
- Maintains core timekeeping even without network
- Provides user feedback on system status
- Enables troubleshooting without complete failure
- Improves system reliability

**Implementation**:
- State machine handles error states
- LCD displays error messages
- Automatic retry mechanisms
- Fallback to local time when NTP fails

### 12. EEPROM-Based Configuration Persistence

**Decision**: Store configuration in EEPROM with validation.

**Rationale**:
- Survives power cycles
- Maintains user settings
- Provides default values for first-time setup
- Enables configuration without reprogramming

**Implementation**:
```cpp
struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool isValid;
};
```

## Performance Considerations

### 13. Memory Management

**Decision**: Careful memory allocation and static allocation where possible.

**Rationale**:
- Limited RAM on Arduino UNO R4 WiFi (32KB)
- Prevents memory fragmentation
- Ensures stable operation over long periods
- Reduces risk of stack overflow

**Implementation**:
- Static allocation for large objects
- Minimal dynamic memory allocation
- Regular memory monitoring in debug builds

### 14. Interrupt Safety

**Decision**: Minimize interrupt service routine complexity.

**Rationale**:
- Prevents interrupt conflicts
- Ensures reliable power-off detection
- Maintains system responsiveness
- Reduces risk of watchdog timeouts

**Implementation**:
- Simple ISR for power-off detection
- Main loop handles complex operations
- Careful interrupt priority management

## Future Considerations

### 15. Extensibility

**Decision**: Design for easy addition of new features.

**Rationale**:
- Supports future enhancements
- Maintains clean architecture
- Enables community contributions
- Reduces technical debt

**Implementation**:
- Clear interfaces between components
- Plugin-like architecture for new features
- Configuration-driven behavior
- Comprehensive documentation

---

*This document should be updated as new design decisions are made or existing ones are modified.* 
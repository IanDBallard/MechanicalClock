# Mechanical Clock with Onboard RTC - Technical Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Class Reference](#class-reference)
4. [State Machine](#state-machine)
5. [API Reference](#api-reference)
6. [Configuration](#configuration)
7. [Troubleshooting](#troubleshooting)
8. [Version History](#version-history)

---

## Project Overview

A sophisticated Arduino UNO R4 WiFi-based mechanical clock system featuring:
- **Onboard RTC** for reliable timekeeping
- **Stepper motor** for precise mechanical hand movement
- **I2C LCD display** for time/date/status display
- **WiFi connectivity** for NTP time synchronization
- **Power-off recovery** with EEPROM state persistence
- **Object-oriented design** with clear separation of concerns

### Key Features
- **Dual Clock System**: Mechanical stepper-driven hands + digital LCD display
- **Automatic NTP Sync**: Hourly synchronization with time servers
- **Power Recovery**: Remembers position after power loss
- **WiFi Configuration**: Captive portal for easy WiFi setup
- **DST Support**: Automatic daylight saving time adjustment
- **Graceful Degradation**: Continues operation without WiFi

---

## Architecture

### Design Principles
- **Single Responsibility**: Each class has one clear purpose
- **Dependency Injection**: Dependencies passed via constructors
- **State Machine Pattern**: Centralized control flow management
- **Inheritance Hierarchy**: Base `Clock` class with specialized implementations
- **Encapsulation**: Private implementation details, clean public interfaces

### Class Hierarchy
```
Clock (abstract base)
├── MechanicalClock (stepper motor control)
└── DigitalClock (LCD display management)

Supporting Classes:
├── StateManager (finite state machine)
├── NetworkManager (WiFi/NTP handling)
├── LCDDisplay (I2C display interface)
├── LED (activity indicator)
├── TimeUtils (time conversion utilities)
└── Constants (centralized configuration)
```

### Data Flow
1. **Initialization**: Hardware setup → State determination → Clock positioning
2. **Normal Operation**: State machine → Clock updates → Display refresh
3. **Network Events**: WiFi status → NTP sync → Time adjustment
4. **Power Events**: ISR detection → EEPROM save → Recovery on restart

---

## Class Reference

### Clock (Base Class)
**Purpose**: Abstract interface for all clock implementations

**Key Methods**:
- `begin()` - Initialize clock hardware
- `updateCurrentTime()` - Unified time synchronization (normal operation + sync events)
- `handlePowerOff()` - ISR-safe power-off handling with EEPROM time saving

**Protected Members**:
- `RTClock& _rtc` - Reference to RTC instance
- `LCDDisplay& _lcd` - Reference to LCD display

**Design Pattern**: Template Method Pattern for `handlePowerOff()` - base class provides common EEPROM saving, derived classes add specific actions

### MechanicalClock
**Purpose**: Controls stepper motor for mechanical hand movement

**Key Features**:
- **Microstepping Support**: Configurable step resolution
- **Position Tracking**: Absolute time position in seconds
- **Shortest Path Movement**: Automatic detection for large time changes
- **Power Recovery**: EEPROM-based position restoration

**Key Methods**:
```cpp
void begin() override;
void updateCurrentTime() override; // Unified method (normal + sync events)
void handlePowerOff() override; // Mechanical-specific (stepper driver, LED)
void setMicrosteppingMode(uint8_t mode);
```

**Movement Logic**:
- **Unified Time Updates**: Single `updateCurrentTime()` handles all time synchronization events
- **Automatic Large Movement Detection**: Time differences >6 hours trigger shortest path calculation
- **Shortest Path Movement**: 12-hour cycle wrap-around for large time changes
- **Power Recovery**: EEPROM-based position restoration in `begin()`
- **Stepper Management**: Automatic enable/disable based on movement activity

### DigitalClock
**Purpose**: Manages LCD display updates

**Key Features**:
- **Optimized Updates**: Only refreshes when values change
- **Display Delegation**: Uses LCDDisplay class for actual I/O
- **Time Synchronization**: Updates display on time sync events

**Key Methods**:
```cpp
void begin() override;
void updateCurrentTime() override; // Unified method (normal + sync events)
// handlePowerOff() inherited from base class (EEPROM time saving only)
```

**Optimization Features**:
- **Display Optimization**: `_lastDisplayed*` variables prevent unnecessary LCD writes
- **Helper Methods**: `updateTrackingVariables()` and `forceDisplayUpdate()` for code reuse
- **Time Component Tracking**: Monitors second, minute, hour, day, month, year changes

### StateManager
**Purpose**: Centralized state machine for system control flow

**States**:
- `STATE_INIT` - Initial system setup
- `STATE_CONFIG` - WiFi configuration mode  
- `STATE_CONNECTING_WIFI` - WiFi connection attempt
- `STATE_SYNCING_TIME` - NTP time synchronization
- `STATE_RUNNING` - Normal clock operation
- `STATE_ERROR` - Error recovery

**Key Methods**:
```cpp
void update() - Main state machine loop
void transitionTo(ClockState newState) - State transitions with validation
ClockState getCurrentState() const - Get current state
void setLastError(const String& error) - Set error message
String getLastError() const - Get last error
void printStateInfo() - Debug state information
```

**State Transitions**:
- **Validated Transitions**: All state changes validated via `_isValidTransition()`
- **Graceful Degradation**: Timeout → Continue without network
- **Error Recovery**: Error state → Retry with delays
- **Automatic Sync**: Running state → Periodic NTP checks

**Timeout Constants**:
```cpp
CONFIG_TIMEOUT_MS = 300000UL        // 5 minutes
WIFI_CONNECT_TIMEOUT_MS = 30000UL   // 30 seconds
NTP_SYNC_TIMEOUT_MS = 30000UL       // 30 seconds
ERROR_DISPLAY_TIMEOUT_MS = 5000UL   // 5 seconds
DEBUG_PRINT_INTERVAL_MS = 300000UL  // 5 minutes
```

**State Validation Rules**:
- `STATE_INIT` → `STATE_CONFIG` or `STATE_CONNECTING_WIFI`
- `STATE_CONFIG` → `STATE_CONNECTING_WIFI` or `STATE_ERROR`
- `STATE_CONNECTING_WIFI` → `STATE_SYNCING_TIME`, `STATE_RUNNING`, or `STATE_ERROR`
- `STATE_SYNCING_TIME` → `STATE_RUNNING` or `STATE_ERROR`
- `STATE_RUNNING` → `STATE_CONNECTING_WIFI`, `STATE_SYNCING_TIME`, or `STATE_ERROR`
- `STATE_ERROR` → `STATE_INIT` (recovery)

### NetworkManager
**Purpose**: WiFi connectivity and NTP time synchronization

**Key Features**:
- **Dual Mode**: Client (WiFi) and Access Point (configuration)
- **Captive Portal**: Web-based WiFi configuration
- **NTP Sync**: Automatic time synchronization
- **EEPROM Storage**: Persistent WiFi credentials
- **DST Support**: Automatic daylight saving time

**Key Methods**:
```cpp
bool begin() - Initialize network hardware
bool connectToWiFi() - Establish WiFi connection
bool syncTimeWithRTC(RTClock& rtc) - Synchronize with NTP server
bool needsConfiguration() - Check if WiFi setup required
void startConfigurationMode() - Enter AP mode
void resetNtpSyncCounter() - Defer next NTP sync
int getTimeZoneOffset() const - Get timezone offset
bool getUseDST() const - Get DST setting
```

**Configuration Portal**:
- **SSID**: "MechanicalClock"
- **Password**: "12345678"
- **IP Address**: 192.168.4.1
- **Configuration**: WiFi SSID, password, timezone, DST

### LCDDisplay
**Purpose**: I2C LCD display interface and management

**Key Features**:
- **Two-Line Display**: Time/date on line 0, status on line 1
- **Optimized Updates**: Prevents unnecessary writes
- **Error Display**: Overlay and timed error messages
- **Status Icons**: WiFi and sync indicators

**Key Methods**:
```cpp
bool begin() - Initialize I2C display with smart buffer
void updateTimeAndDate(const RTCTime& time) - Update time/date area (columns 0-14)
void updateNetworkStatus(bool wifiConnected, bool ntpSynced) - Update status icons (column 15)
void printLine(uint8_t line, const String& message) - Print message to line (columns 0-14)
```

**Smart Buffer System**:
- **In-Memory Buffer**: `_buffer[LCD_HEIGHT][LCD_WIDTH]` stores current display state
- **Dirty Flag Tracking**: `_lineDirty[]` and `_charDirty[][]` track changes
- **Constrained Areas**: Each method writes only to its designated area
- **Flicker Reduction**: Only changed characters are written to physical LCD
- **Artifact Elimination**: No need for clearing before writing

**Display Layout**:
```
Line 0: [Time/Date Area 0-14] [WiFi Icon 15]
Line 1: [Message Area 0-14]   [Sync Icon 15]
```

**Area Constraints**:
- **Time/Date/Message**: Columns 0-14 (15 characters)
- **Status Icons**: Column 15 only (1 character per line)

### LED
**Purpose**: Activity indicator LED control

**Key Features**:
- **Simple Interface**: On/off control
- **Activity Indication**: Shows stepper motor activity
- **ISR Safe**: No blocking operations

**Key Methods**:
```cpp
void begin() - Initialize LED pin
void on() - Turn LED on
void off() - Turn LED off
```

### Constants
**Purpose**: Centralized configuration and constants management

**Key Categories**:
- **EEPROM Addresses**: All EEPROM storage locations
- **Hardware Pins**: All pin definitions for stepper, LCD, LED, power detection
- **Network Configuration**: WiFi, NTP, and timezone settings
- **Timing Constants**: Timeouts, intervals, and delays
- **Stepper Configuration**: Microstepping modes and step calculations

**Key Constants**:
```cpp
// EEPROM Addresses
EEPROM_ADDRESS_WIFI_CREDENTIALS = 0
EEPROM_ADDRESS_POWER_DOWN_TIME = 32

// Hardware Pins
STEP_PIN = 8, DIR_PIN = 7, ENABLE_PIN = 3
MS1_PIN = 4, MS2_PIN = 5, MS3_PIN = 6
LED_PIN = 13, POWER_PIN = 2

// Network Configuration
AP_SSID = "ClockSetup"
NTP_SYNC_INTERVAL = 3600000UL // 1 hour
TIME_ZONE_OFFSET_HOURS = -4
USE_DST_AUTO_CALC = true
```

### TimeUtils
**Purpose**: Time conversion and DST calculation utilities

**Key Functions**:
```cpp
time_t getCurrentUTC() - Get current UTC time
time_t convertUTCToLocal(time_t utcTime, int timeZoneOffset, bool useDST)
time_t convertLocalToUTC(time_t localTime, int timeZoneOffset, bool useDST)
bool calculateDST(RTCTime& utcTime, int timeZoneOffsetHours)
int Month2int(Month month) - Convert Month enum to int
int DayOfWeek2int(DayOfWeek day, bool sundayFirst) - Convert DayOfWeek enum to int
```

**DST Logic**:
- **US Rules**: Second Sunday in March, First Sunday in November
- **Automatic Detection**: Based on date and timezone
- **UTC-First Strategy**: All calculations start from UTC

---

## State Machine

### State Flow Diagram
```
[POWER ON]
    ↓
STATE_INIT
    ↓
[WiFi Configured?] → NO → STATE_CONFIG
    ↓ YES
STATE_CONNECTING_WIFI
    ↓
[WiFi Connected?] → NO → STATE_RUNNING (graceful degradation)
    ↓ YES
STATE_SYNCING_TIME
    ↓
[NTP Sync Success?] → NO → STATE_RUNNING (graceful degradation)
    ↓ YES
STATE_RUNNING
    ↓
[Periodic Checks] → [WiFi Lost?] → STATE_CONNECTING_WIFI
    ↓
[Error Condition] → STATE_ERROR → [Recovery] → STATE_INIT
```

### State Details

#### STATE_INIT
- **Purpose**: System initialization
- **Duration**: Brief, transitions immediately
- **Actions**: Hardware setup, state determination

#### STATE_CONFIG
- **Purpose**: WiFi configuration via captive portal
- **Duration**: Until configuration complete or timeout
- **Actions**: AP mode, web server, credential storage

#### STATE_CONNECTING_WIFI
- **Purpose**: Establish WiFi connection
- **Duration**: Up to 30 seconds
- **Actions**: WiFi connection attempt, timeout handling

#### STATE_SYNCING_TIME
- **Purpose**: NTP time synchronization
- **Duration**: Up to 30 seconds
- **Actions**: NTP request, time adjustment, timeout handling

#### STATE_RUNNING
- **Purpose**: Normal clock operation
- **Duration**: Continuous until state change
- **Actions**: Clock updates, periodic NTP checks, display refresh

#### STATE_ERROR
- **Purpose**: Error recovery
- **Duration**: 5 seconds
- **Actions**: Error display, recovery attempt

---

## API Reference

### Constants
```cpp
// EEPROM Addresses
#define EEPROM_ADDRESS_INITIAL_TIME 0
#define EEPROM_ADDRESS_WIFI_CREDENTIALS 4
#define EEPROM_ADDRESS_SYSTEM_STATE 100

// Hardware Pins
#define POWER_PIN 2
#define STEP_PIN 8
#define DIR_PIN 7
#define ENABLE_PIN 3
#define MS1_PIN 4
#define MS2_PIN 5
#define MS3_PIN 6
#define LED_PIN 13

// Timing Constants
#define WIFI_CONNECT_TIMEOUT 30000UL
#define NTP_SYNC_TIMEOUT 30000UL
#define CONFIG_TIMEOUT 300000UL
#define ERROR_RECOVERY_DELAY 5000UL
#define NTP_SYNC_INTERVAL 3600000UL

// Network Configuration
#define AP_SSID "MechanicalClock"
#define AP_PASSWORD "12345678"
```

### Time Constants
```cpp
#define SECONDS_IN_12_HOURS 43200L
#define SECONDS_IN_HOUR 3600L
#define SECONDS_IN_MINUTE 60L
```

### Error Codes
- **WiFi Connection Failed**: Graceful degradation to RTC-only mode
- **NTP Sync Failed**: Continue with RTC time
- **LCD Communication Error**: System continues without display
- **Stepper Motor Error**: System stops, requires manual intervention

---

## Configuration

### Hardware Setup
1. **Stepper Motor**: Connect to pins 8 (STEP), 7 (DIR), 3 (ENABLE)
2. **Microstepping**: Configure MS1, MS2, MS3 pins (4, 5, 6)
3. **LCD Display**: Connect to I2C (SDA, SCL)
4. **Power Detection**: Connect to pin 2 with pull-up resistor
5. **Activity LED**: Connect to pin 13

### WiFi Configuration
1. **First Boot**: Device creates "MechanicalClock" access point
2. **Connect**: Join "MechanicalClock" network (password: 12345678)
3. **Configure**: Navigate to 192.168.4.1 in web browser
4. **Enter Details**: WiFi SSID, password, timezone, DST preference
5. **Save**: Device reboots and connects to configured network

### Timezone Settings
- **Offset**: Hours from UTC (e.g., -5 for EST, -8 for PST)
- **DST**: Automatic calculation or manual override
- **NTP Server**: Default: time.nist.gov (129.6.15.28)

---

## Troubleshooting

### Common Issues

#### Clock Hands Not Moving
- **Check**: Stepper motor connections
- **Verify**: Power supply adequacy
- **Debug**: Check serial output for movement commands

#### LCD Display Issues
- **Check**: I2C connections and address (default: 0x27)
- **Verify**: Power supply to LCD module
- **Debug**: Use I2C scanner to find correct address

#### WiFi Connection Problems
- **Check**: Credentials in configuration portal
- **Verify**: Network availability and signal strength
- **Debug**: Serial output shows connection attempts

#### Time Synchronization Issues
- **Check**: Internet connectivity
- **Verify**: NTP server accessibility
- **Debug**: Serial output shows sync attempts and results

#### Power Recovery Problems
- **Check**: EEPROM write permissions
- **Verify**: Power detection circuit
- **Debug**: Serial output shows recovery calculations

### Debug Output
Enable debug output by checking serial monitor at 115200 baud:
- **Movement Commands**: Stepper motor calculations
- **Network Status**: WiFi connection and NTP sync attempts
- **State Transitions**: State machine flow
- **Time Calculations**: UTC/local conversions and DST

### Recovery Procedures
1. **Factory Reset**: Clear EEPROM to reset all settings
2. **Manual Position**: Use stepper control commands for hand alignment
3. **Network Reset**: Re-enter configuration mode for WiFi issues
4. **Time Reset**: Force NTP sync or manual time setting

---

## Version History

### V2.1.15 (Current)
- **StateManager Cleanup**: Removed unused `STATE_POWER_SAVING` state
- **State Validation**: Added `_isValidTransition()` method for robust state changes
- **Timeout Constants**: Centralized all timeout values with named constants
- **Error Handling**: Fixed static variable issue in error state recovery
- **Dependencies**: Added missing `TimeUtils.h` include
- **Code Quality**: 29 lines removed, 60 lines added for improved functionality

### V2.1.14
- **NetworkManager Cleanup**: Removed unused `syncTimeWithNTP()` placeholder method
- **Interface Cleanup**: Eliminated dead code that could mislead developers
- **StateManager Integration**: Confirmed proper use of `syncTimeWithRTC()` method

### V2.1.13
- **LCD Smart Buffer**: Implemented "Smart Buffer with Region Tracking" system
- **Constrained Areas**: Each display method writes only to its designated area
- **Flicker Reduction**: Only changed characters written to physical LCD
- **Error System Removal**: Eliminated unused `displayError()` system and `_errorDisplayed` flag
- **Artifact Elimination**: No more clearing artifacts on LCD updates

### V2.1.12
- **Method Consolidation**: Merged `update()` and `updateCurrentTime()` into single unified method
- **Simplified Interface**: Both `MechanicalClock` and `DigitalClock` use single update method
- **StateManager Update**: Updated to use unified `updateCurrentTime()` method
- **Code Reduction**: Eliminated duplicate logic between update methods

### V2.1.11
- **Code Reuse**: Extracted common logic into helper methods for `DigitalClock`
- **Helper Methods**: `updateTrackingVariables()` and `forceDisplayUpdate()`
- **DRY Principle**: Eliminated code duplication in time extraction and tracking

### V2.1.10
- **Template Method Pattern**: Moved common EEPROM saving to base `Clock` class
- **Inheritance Optimization**: `MechanicalClock` overrides for stepper/LED handling
- **Code Reuse**: `DigitalClock` inherits base implementation
- **OOP Best Practices**: Proper use of inheritance and polymorphism

### V2.1.9
- **Display Optimization**: Implemented actual display optimization using `_lastDisplayed*` variables
- **LCD Efficiency**: Prevents unnecessary LCD writes when values haven't changed
- **Method Refactoring**: Updated `update()` and `updateCurrentTime()` for optimization
- **RTCTime Fixes**: Corrected method calls to use proper `RTCTime` getters

### V2.1.8
- **Documentation Structure**: Created comprehensive `DOCUMENTATION.md` file
- **Best Practices**: Added development guidelines and coding standards
- **Version Tracking**: Enhanced change tracking and documentation

### V2.1.7
- **Unified Time Sync**: Consolidated `updateCurrentTime()` method
- **Removed Redundancy**: Eliminated `adjustToInitialTime()` method
- **Automatic Detection**: Large movements trigger shortest path calculation
- **Code Reduction**: 44 lines removed, improved maintainability

### V2.1.6
- **Unified Architecture**: Re-introduced `updateCurrentTime()` design
- **OOP Compliance**: Proper inheritance and polymorphism
- **State Integration**: StateManager uses unified time sync
- **Documentation**: Updated method documentation

### V2.1.5
- **Documentation Updates**: Comprehensive V2.1.x change documentation
- **Code Quality**: Improved comments and structure
- **Version Tracking**: Enhanced change tracking

### V2.1.4
- **Power Recovery Refactor**: Moved logic to `MechanicalClock::begin()`
- **Simplified Initialization**: Removed complex reset detection
- **EEPROM-Based Recovery**: Reliable power-down time storage

### V2.1.3
- **Constants Centralization**: Created `Constants.h` for all magic numbers
- **Clean Architecture**: Removed hardcoded values from `main.cpp`
- **Maintainability**: Single source for all configuration constants

### V2.1.2
- **Graceful Network Handling**: Prevents infinite WiFi reconnection loops
- **NTP Sync Resilience**: Continues operation without network
- **Timeout Management**: Proper timeout handling in all network states

### V2.1.1
- **LCD Display Fixes**: Resolved "clock running" persistence issue
- **Day of Week Correction**: Fixed off-by-one day calculation
- **Debug Output Cleanup**: Removed excessive serial output

### V2.0 (Base)
- **Object-Oriented Design**: Complete refactor to OOP architecture
- **State Machine**: Centralized control flow management
- **Network Manager**: Comprehensive WiFi and NTP handling
- **Power Recovery**: EEPROM-based state persistence
- **Dual Clock System**: Mechanical and digital clock implementations

---

## Development Guidelines

### Code Style
- **Naming**: camelCase for methods, snake_case for variables
- **Comments**: Comprehensive documentation for complex logic
- **Error Handling**: Graceful degradation over hard failures
- **Memory**: Efficient use of limited Arduino resources

### Testing Strategy
- **Unit Tests**: Individual class testing in test environment
- **Integration Tests**: Full system testing on hardware
- **Power Tests**: Power-off recovery validation
- **Network Tests**: WiFi and NTP functionality verification

### Future Enhancements
- **Web Interface**: Real-time status and control via web browser
- **Multiple Timezones**: Support for multiple timezone displays
- **Alarm Functionality**: Time-based alarm system
- **Weather Integration**: Display weather information
- **Mobile App**: Bluetooth or WiFi-based mobile control

---

*Last Updated: V2.1.15*
*Documentation Version: 1.1* 
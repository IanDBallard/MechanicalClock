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
└── TimeUtils (time conversion utilities)
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
- `update()` - Main operational loop
- `updateCurrentTime()` - Synchronize to current time
- `handlePowerOff()` - ISR-safe power-off handling

**Protected Members**:
- `RTClock& _rtc` - Reference to RTC instance
- `LCDDisplay& _lcd` - Reference to LCD display

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
void update() override;
void updateCurrentTime() override;
void handlePowerOff() override;
void setMicrosteppingMode(uint8_t mode);
```

**Movement Logic**:
- **Normal Movement**: Direct time difference calculation
- **Large Movement (>6h)**: Shortest path with 12-hour cycle wrap-around
- **Power Recovery**: Sets initial position, then syncs to current time

### DigitalClock
**Purpose**: Manages LCD display updates

**Key Features**:
- **Optimized Updates**: Only refreshes when values change
- **Display Delegation**: Uses LCDDisplay class for actual I/O
- **Time Synchronization**: Updates display on time sync events

**Key Methods**:
```cpp
void begin() override;
void update() override;
void updateCurrentTime() override;
void handlePowerOff() override;
```

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
void transitionTo(ClockState newState) - State transitions
bool isInState(ClockState state) - State checking
```

**State Transitions**:
- **Graceful Degradation**: Timeout → Continue without network
- **Error Recovery**: Error state → Retry with delays
- **Automatic Sync**: Running state → Periodic NTP checks

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
bool syncTimeWithNTP() - Synchronize with NTP server
bool needsConfiguration() - Check if WiFi setup required
void startConfigurationMode() - Enter AP mode
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
bool begin() - Initialize I2C display
void updateTimeAndDate(const RTCTime& time) - Update time display
void updateNetworkStatus(bool wifiConnected, bool ntpSynced) - Update status
void displayError(const String& message, bool overlay, unsigned long duration)
```

**Display Layout**:
```
Line 0: [Time] [Date] [Day of Week]
Line 1: [WiFi Icon] [Sync Icon] [Status Message]
```

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

### V2.1.7 (Current)
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

*Last Updated: V2.1.7*
*Documentation Version: 1.0* 
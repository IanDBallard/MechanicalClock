# Power-Up and Power-Down Test Coverage

## Overview

This document outlines the comprehensive test coverage for power-up and power-down scenarios in the Mechanical Clock project. These tests ensure robust recovery from various power interruption scenarios.

## Test Categories

### 1. Reset Cause Detection Tests

**Purpose**: Verify that the system correctly identifies different types of resets and chooses appropriate recovery strategies.

**Test Scenarios**:
- **Power-On Reset (PORF)**: Tests detection of power-on reset flags
- **Software Reset (SWRF)**: Tests detection of software-initiated resets
- **Watchdog Reset (WDTRF)**: Tests detection of watchdog timer resets
- **External Reset (CWSF)**: Tests detection of external reset button/upload resets

**Expected Behavior**:
- Power-related resets should use EEPROM time for recovery
- Non-power resets should use current RTC time

### 2. EEPROM Recovery Tests

**Purpose**: Validate EEPROM data integrity and recovery mechanisms.

**Test Scenarios**:
- **Valid Time Recovery**: Tests recovery of recent, valid timestamps
- **Invalid Time Recovery**: Tests handling of old/corrupted timestamps
- **Zero Time Recovery**: Tests handling of uninitialized EEPROM
- **Corrupted Time Recovery**: Tests handling of negative/invalid timestamps

**Validation Logic**:
- Timestamps before Jan 1, 2023 are considered invalid
- Zero timestamps indicate uninitialized EEPROM
- Negative timestamps indicate corruption

### 3. Time Recovery Logic Tests

**Purpose**: Verify correct time source selection based on reset type.

**Test Scenarios**:
- **Power-Related Reset**: Should use EEPROM time
- **Software Reset**: Should use RTC time
- **External Reset**: Should use RTC time
- **Watchdog Reset**: Should use RTC time

**Time Difference Calculation**:
- Tests calculation of time elapsed during power loss
- Validates step calculation for mechanical hand movement

### 4. Mechanical Clock Power Recovery Tests

**Purpose**: Ensure stepper motor and mechanical components recover correctly.

**Test Scenarios**:
- **Stepper Motor State**: Should be disabled initially after power-up
- **LED State**: Activity LED should be off initially
- **Hand Position Calculation**: Tests step calculation for time difference
- **Microstepping Mode**: Verifies mode persistence across power cycles

**Recovery Process**:
- Calculate time difference between saved and current time
- Convert time difference to stepper motor steps
- Move hands to correct position

### 5. Network Recovery After Power-Up Tests

**Purpose**: Verify network configuration persistence and recovery.

**Test Scenarios**:
- **WiFi Credentials**: Tests persistence of SSID and password
- **Timezone Settings**: Tests persistence of timezone offset
- **DST Settings**: Tests persistence of DST configuration
- **NTP Sync**: Tests automatic NTP synchronization after power-up

**Validation**:
- Credentials should be valid and non-empty
- Timezone should be within valid range (-12 to +14)
- DST setting should be boolean (true/false)

### 6. State Recovery After Power-Up Tests

**Purpose**: Ensure system state transitions correctly after power restoration.

**Test Scenarios**:
- **Running State Recovery**: Should resume normal operation
- **Error State Recovery**: Should restart from initialization
- **Configuration State Recovery**: Should restart configuration process
- **WiFi Connection Recovery**: Should retry WiFi connection

**State Machine Logic**:
- Each state should have appropriate recovery behavior
- Error states should trigger restart procedures
- Normal states should resume operation

### 7. Power-Down Scenarios Tests

**Purpose**: Verify appropriate behavior during different power-down scenarios.

**Test Scenarios**:
- **Power-Down During Running**: Should save current time
- **Power-Down During Error**: Should not save time (system unstable)
- **Power-Down During Configuration**: Should not save time (not fully operational)
- **Power-Down During WiFi Connection**: Should not save time (not synchronized)

**Save Logic**:
- Only save time when system is in stable running state
- Avoid saving time during error or initialization states

### 8. EEPROM Corruption Scenarios Tests

**Purpose**: Test system resilience to corrupted EEPROM data.

**Test Scenarios**:
- **Corrupted WiFi Credentials**: Tests handling of invalid SSID/password
- **Corrupted Timezone Data**: Tests handling of invalid timezone values
- **Corrupted DST Data**: Tests handling of invalid DST settings

**Recovery Mechanisms**:
- Invalid credentials should trigger configuration mode
- Invalid timezone should use default values
- Invalid DST should use default setting

### 9. Power-Up Timing Scenarios Tests

**Purpose**: Test system behavior after different power loss durations.

**Test Scenarios**:
- **Short Power Loss (seconds)**: Tests recovery from brief interruptions
- **Medium Power Loss (minutes)**: Tests recovery from moderate interruptions
- **Long Power Loss (days)**: Tests recovery from extended interruptions

**Handling Logic**:
- Short losses: Minimal recovery needed
- Medium losses: Standard recovery procedures
- Long losses: Full reinitialization may be required

### 10. Stepper Motor Recovery Scenarios Tests

**Purpose**: Verify stepper motor state recovery after power loss.

**Test Scenarios**:
- **Position Recovery**: Tests restoration of motor position
- **Direction Recovery**: Tests restoration of motor direction
- **Speed Recovery**: Tests restoration of motor speed settings

**Recovery Process**:
- Reset motor to known position
- Restore direction and speed settings
- Move to correct time position

### 11. LCD Display Recovery Scenarios Tests

**Purpose**: Ensure LCD display recovers correctly after power loss.

**Test Scenarios**:
- **LCD Initialization**: Tests reinitialization after power-up
- **Backlight State**: Tests backlight restoration
- **Display Content**: Tests content restoration

**Recovery Process**:
- Reinitialize I2C communication
- Restore backlight state
- Redisplay current time and status

## Test Implementation

### Desktop Tests (Mock Environment)
- **Location**: `test_desktop/run_tests_desktop.cpp`
- **Purpose**: Fast validation of logic without hardware
- **Coverage**: All test categories with mock implementations

### Arduino Tests (Hardware Environment)
- **Location**: `test/PowerUpTest.cpp`
- **Purpose**: Hardware-specific validation
- **Coverage**: All test categories with real hardware interaction

## Running the Tests

### Desktop Tests
```bash
cd test_desktop
g++ -std=c++17 -I. run_tests_desktop.cpp -o run_tests_desktop
./run_tests_desktop
```

### Arduino Tests
```bash
# Set ARDUINO_TESTING flag in platformio.ini
pio run --environment uno_r4_wifi_test --target upload
pio device monitor
```

## Test Results

### Current Coverage
- ✅ **20 Desktop Tests**: All passing
- ✅ **11 Arduino Test Categories**: Comprehensive coverage
- ✅ **Reset Cause Detection**: Full coverage
- ✅ **EEPROM Recovery**: Full coverage
- ✅ **Time Recovery Logic**: Full coverage
- ✅ **Mechanical Recovery**: Full coverage
- ✅ **Network Recovery**: Full coverage
- ✅ **State Recovery**: Full coverage
- ✅ **Power-Down Scenarios**: Full coverage
- ✅ **Corruption Handling**: Full coverage
- ✅ **Timing Scenarios**: Full coverage

## Future Enhancements

### Additional Test Scenarios
1. **Battery Backup Tests**: Test RTC battery backup functionality
2. **Brown-Out Recovery**: Test recovery from voltage fluctuations
3. **Partial Write Scenarios**: Test EEPROM partial write corruption
4. **Concurrent Power Events**: Test multiple power events in sequence

### Hardware-Specific Tests
1. **Real Power Interruption**: Test with actual power loss simulation
2. **Temperature Effects**: Test recovery at different temperatures
3. **Voltage Variations**: Test recovery with varying supply voltages

## Conclusion

The power-up and power-down test suite provides comprehensive coverage of all critical recovery scenarios. The dual implementation (desktop and Arduino) ensures both logical correctness and hardware compatibility. This robust testing framework significantly improves the reliability of the mechanical clock system in real-world power interruption scenarios. 
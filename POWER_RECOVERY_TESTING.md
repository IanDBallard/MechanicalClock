# Power Recovery Testing Guide

## Overview

The mechanical clock project now includes an improved power recovery system that can be tested without physically disconnecting the USB cable. This makes debugging and validation much easier.

## Problem Solved

### **Original Issue:**
- Power recovery testing required physical USB disconnection
- Hard to verify EEPROM writes without actual power loss
- No way to simulate different power-down scenarios
- Difficult to debug recovery logic

### **Solution:**
- **Software simulation** of power-off events
- **Data validation** with magic numbers and range checking
- **Test mode detection** for simulation vs. real power loss
- **Comprehensive debugging** output during recovery

## How It Works

### **1. Enhanced EEPROM Storage**
The system now saves more information during power-off:

```cpp
// Power-off data structure
EEPROM.put(EEPROM_ADDRESS_INITIAL_TIME, timeToSave);     // Timestamp
EEPROM.put(EEPROM_ADDRESS_POWER_STATE, POWER_STATE_RUNNING); // System state
EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, RECOVERY_VALIDATION_MAGIC); // Validation
EEPROM.put(EEPROM_ADDRESS_TEST_MODE, POWER_STATE_TEST);  // Test mode flag
```

### **2. Data Validation**
Recovery data is validated using:
- **Magic number** (`0xDEADBEEF`) to detect corruption
- **Timestamp range** validation (Jan 1, 2023 to Jan 1, 2100)
- **State validation** (valid power states only)

### **3. Test Mode Detection**
The system can distinguish between:
- **Real power loss** (ISR-triggered save)
- **Test simulation** (software-triggered save)

## Testing Methods

### **Method 1: Software Simulation**
```cpp
// Simulate power-off without physical disconnection
clock.simulatePowerOff(POWER_STATE_RUNNING);
```

### **Method 2: Data Validation**
```cpp
// Validate saved data integrity
bool isValid = clock.validatePowerRecoveryData();
```

### **Method 3: Recovery Testing**
```cpp
// Simulate power-on recovery
clock.begin(); // Detects and processes saved data
```

## Available Test Commands

### **Basic Testing**
- `sim` - Simulate power-off
- `rec` - Simulate power-on recovery
- `val` - Validate saved data
- `clr` - Clear saved data
- `info` - Show saved data info

### **Advanced Testing**
- Test different power states (running, error, config)
- Test data corruption scenarios
- Test validation logic
- Test recovery process

## Test Files

### **1. PowerRecoveryTest.cpp**
Comprehensive test suite for power recovery functionality:
- Power-off simulation tests
- Data validation tests
- Recovery process tests
- Different state scenarios
- Data clearing tests

### **2. PowerRecoveryDemo.cpp**
Demonstration program showing how to use the system:
- Step-by-step testing guide
- Interactive testing interface
- Different scenario demonstrations
- Data validation examples

## Usage Examples

### **Example 1: Basic Power Recovery Test**
```cpp
// Initialize clock
MechanicalClock clock(8, 7, 3, 4, 5, 6, 13, rtcInstance, lcdDisplay);
clock.begin();

// Simulate power-off
clock.simulatePowerOff(POWER_STATE_RUNNING);

// Verify data was saved
time_t savedTime = clock.getPowerDownTime();
bool isValid = clock.validatePowerRecoveryData();

// Simulate power-on recovery
MechanicalClock clock2(8, 7, 3, 4, 5, 6, 13, rtcInstance2, lcdDisplay2);
clock2.begin(); // Should detect saved data

// Clean up
clock2.clearPowerRecoveryData();
```

### **Example 2: Testing Different States**
```cpp
// Test power-off during running state
clock.simulatePowerOff(POWER_STATE_RUNNING);

// Test power-off during error state
clock.simulatePowerOff(POWER_STATE_ERROR);

// Test power-off during configuration
clock.simulatePowerOff(POWER_STATE_CONFIG);
```

### **Example 3: Data Corruption Testing**
```cpp
// Save valid data
clock.simulatePowerOff(POWER_STATE_RUNNING);

// Corrupt the validation flag
EEPROM.put(EEPROM_ADDRESS_RECOVERY_FLAG, 0x12345678);

// Test validation (should fail)
bool isValid = clock.validatePowerRecoveryData(); // Returns false
```

## Debug Output

The system provides comprehensive debug output:

```
=== POWER RECOVERY ANALYSIS ===
Power-down time from EEPROM: 1704067200
Power-down state: 1
Test mode: YES
✓ Valid power-down time found - will calculate stepper adjustment after NTP sync
✓ Cleared saved power recovery data from EEPROM.
=== TEST MODE DETECTED ===
Power recovery simulation successful!
Clock will adjust position after NTP sync.
=== POWER RECOVERY ANALYSIS COMPLETE ===
```

## Benefits

### **1. Easy Testing**
- No physical USB disconnection required
- Can test multiple scenarios quickly
- Immediate feedback on test results

### **2. Robust Validation**
- Detects corrupted or invalid data
- Validates timestamp ranges
- Checks state consistency

### **3. Better Debugging**
- Clear debug output during recovery
- Test mode detection
- Step-by-step process visibility

### **4. Comprehensive Coverage**
- Tests all power states
- Tests data corruption scenarios
- Tests recovery process

## Integration with Existing System

The improved power recovery system is fully backward compatible:
- **Existing ISR** still works for real power loss
- **New simulation methods** available for testing
- **Enhanced validation** improves reliability
- **Better debugging** helps with troubleshooting

## Running Tests

### **Build and Run Tests**
```bash
# Build test environment
pio run -e uno_r4_wifi_test

# Run specific power recovery tests
# (Use the test runner scripts)
```

### **Monitor Output**
```bash
# Monitor test output
pio device monitor
```

## Conclusion

The improved power recovery system makes testing much easier and more reliable. You can now:
- Test power recovery without physical disconnection
- Validate data integrity automatically
- Debug recovery issues more effectively
- Test multiple scenarios quickly

This significantly improves the development and testing experience for power recovery functionality.
# Individual Test Environments

## Overview
This project uses a **working test runner script** that dynamically configures the test environment to run individual test suites. The individual PlatformIO environments were removed due to compilation issues.

## Available Test Suites

### 1. **TimeUtilsTest** - TimeUtils Test Suite
- **Purpose**: Tests time conversion, DST logic, and time utilities
- **Script**: `./run_single_test_pio.sh 1`
- **Files**: `TimeUtilsTest.cpp`

### 2. **LEDTest** - LED Test Suite  
- **Purpose**: Tests LED state management and pin control
- **Script**: `./run_single_test_pio.sh 2`
- **Files**: `LEDTest.cpp`

### 3. **NetworkManagerTest** - NetworkManager Test Suite
- **Purpose**: Tests WiFi, NTP, and timezone functionality
- **Script**: `./run_single_test_pio.sh 3`
- **Files**: `NetworkManagerTest.cpp`

### 4. **StateManagerTest** - StateManager Test Suite
- **Purpose**: Tests state machine and transitions
- **Script**: `./run_single_test_pio.sh 4`
- **Files**: `StateManagerTest.cpp`

### 5. **PowerUpTest** - PowerUp Test Suite
- **Purpose**: Tests power-up scenarios and recovery
- **Script**: `./run_single_test_pio.sh 5`
- **Files**: `PowerUpTest.cpp`

### 6. **PowerOffRecoveryTest** - PowerOffRecovery Test Suite
- **Purpose**: Tests power-off recovery logic
- **Script**: `./run_single_test_pio.sh 6`
- **Files**: `PowerOffRecoveryTest.cpp`

## How to Use

### Method 1: Test Runner Script (Recommended)
```bash
# Run a specific test suite
./run_single_test_pio.sh 1  # TimeUtils
./run_single_test_pio.sh 2  # LED
./run_single_test_pio.sh 3  # NetworkManager
# etc.

# Then build and upload
pio run --environment uno_r4_wifi_test --target upload
pio device monitor
```

### Method 2: Interactive Selection
```bash
# Run the script without arguments for interactive menu
./run_single_test_pio.sh
```

### Method 3: Full Test Cycle
```bash
# Configure, build, upload, and monitor in sequence
./run_single_test_pio.sh 1 && pio run --environment uno_r4_wifi_test --target upload && pio device monitor
```

## Benefits

✅ **Isolated Testing**: Each test runs independently without conflicts  
✅ **Easy Selection**: Choose specific tests from PIO menu  
✅ **No File Manipulation**: No need to rename/hide files  
✅ **Reliable**: Uses PlatformIO's native environment system  
✅ **Clear Output**: Each test has its own dedicated runner  

## Troubleshooting

### Test Hangs
- Press the **Reset** button on the Arduino
- Check that the correct environment is selected
- Ensure no other serial monitor is open

### Compilation Errors
- Verify all test files exist in the `test/` directory
- Check that the test setup functions are properly declared
- Ensure no duplicate `main()` functions

### No Test Output
- Open the serial monitor after upload
- Check baud rate (115200)
- Verify the test environment includes the correct test runner

## Legacy Test Environment

The original `uno_r4_wifi_test` environment still exists for running all tests together, but individual environments are recommended for debugging specific issues. 
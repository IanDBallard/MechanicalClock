#!/bin/bash

# Test Runner Script for Mechanical Clock Project
# Runs each test suite individually to isolate issues

echo "=========================================="
echo "    MECHANICAL CLOCK TEST RUNNER"
echo "=========================================="
echo ""

# Function to run a single test
run_single_test() {
    local test_name=$1
    local test_file=$2
    
    echo "Running test: $test_name"
    echo "----------------------------------------"
    
    # Create a temporary test runner for this specific test
    cat > test/TempTestRunner.cpp << EOF
#include "TestFramework.h"

// Declare the specific test setup function
extern void $test_file();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           SINGLE TEST RUNNER");
    Serial.println("==========================================");
    Serial.println("Running: $test_name");
    Serial.println("");
    
    $test_file();
    testRegistry.runAllTests();
    
    Serial.println("==========================================");
    Serial.println("           TEST COMPLETED");
    Serial.println("==========================================");
}

void loop() {
    delay(1000);
}
EOF

    # Update platformio_test.ini to use this specific test
    cat > test/platformio_test.ini << EOF
[env:uno_r4_wifi_test]
platform = arduino
board = uno_r4_wifi
framework = arduino

; Test-specific build flags
build_flags = 
    -DARDUINO_TESTING=1
    -DTEST_VERBOSE=1

; Libraries needed for testing
lib_deps = 
    AccelStepper
    LiquidCrystal_I2C

; Monitor settings for test output
monitor_speed = 115200
monitor_filters = 
    time
    colorize

; Upload settings
upload_speed = 921600

; Test environment configuration - use specific test
build_src_filter = +<*> -<../src/main.cpp> -<main.cpp> -<TestRunner.cpp> -<TestRunnerMinimal.cpp> -<MinimalTest.cpp> -<TestFramework.cpp> +<TempTestRunner.cpp> +<$3>
EOF

    # Run the test
    echo "Building and uploading test..."
    pio test --environment uno_r4_wifi_test --upload-port COM5
    
    echo ""
    echo "Test completed: $test_name"
    echo "----------------------------------------"
    echo ""
}

# List of tests to run
echo "Available tests:"
echo "1. TimeUtilsTest"
echo "2. LEDTest" 
echo "3. NetworkManagerTest"
echo "4. StateManagerTest"
echo "5. PowerUpTest"
echo "6. PowerOffRecoveryTest"
echo ""

# Run each test individually
run_single_test "TimeUtilsTest" "setupTimeUtilsTests" "TimeUtilsTest.cpp"
run_single_test "LEDTest" "setupLEDTests" "LEDTest.cpp"
run_single_test "NetworkManagerTest" "setupNetworkManagerTests" "NetworkManagerTest.cpp"
run_single_test "StateManagerTest" "setupStateManagerTests" "StateManagerTest.cpp"
run_single_test "PowerUpTest" "setupPowerUpTests" "PowerUpTest.cpp"
run_single_test "PowerOffRecoveryTest" "setupPowerOffRecoveryTests" "PowerOffRecoveryTest.cpp"

echo "=========================================="
echo "    ALL TESTS COMPLETED"
echo "=========================================="

# Clean up temporary files
rm -f test/TempTestRunner.cpp 
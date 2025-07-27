#!/bin/bash

# Test Runner with Monitor for Mechanical Clock Project
# Usage: ./run_test_monitor.sh [test_number]

echo "=========================================="
echo "    TEST RUNNER WITH MONITOR"
echo "=========================================="
echo ""

# List of available tests
tests=(
    "TimeUtilsTest:setupTimeUtilsTests:TimeUtilsTest.cpp"
    "LEDTest:setupLEDTests:LEDTest.cpp"
    "NetworkManagerTest:setupNetworkManagerTests:NetworkManagerTest.cpp"
    "StateManagerTest:setupStateManagerTests:StateManagerTest.cpp"
    "PowerUpTest:setupPowerUpTests:PowerUpTest.cpp"
    "PowerOffRecoveryTest:setupPowerOffRecoveryTests:PowerOffRecoveryTest.cpp"
)

# Show available tests
echo "Available tests:"
for i in "${!tests[@]}"; do
    IFS=':' read -r name setup_func file <<< "${tests[$i]}"
    echo "$((i+1)). $name"
done
echo ""

# Get test selection
if [ $# -eq 0 ]; then
    echo "Enter test number (1-${#tests[@]}): "
    read test_num
else
    test_num=$1
fi

# Validate input
if ! [[ "$test_num" =~ ^[0-9]+$ ]] || [ "$test_num" -lt 1 ] || [ "$test_num" -gt "${#tests[@]}" ]; then
    echo "Invalid test number. Please enter a number between 1 and ${#tests[@]}"
    exit 1
fi

# Get selected test
selected_test="${tests[$((test_num-1))]}"
IFS=':' read -r test_name setup_func test_file <<< "$selected_test"

echo "Running test: $test_name"
echo "----------------------------------------"

# Create backup of current test directory
echo "Creating backup of test directory..."
cp -r test test_backup_$$

# Hide other test files temporarily
echo "Hiding other test files..."
cd test
for test_item in "${tests[@]}"; do
    IFS=':' read -r item_name item_setup item_file <<< "$test_item"
    if [ "$item_file" != "$test_file" ]; then
        if [ -f "$item_file" ]; then
            mv "$item_file" "$item_file.bak"
        fi
    fi
done

# Hide other test runners
if [ -f "TestRunner.cpp" ]; then
    mv TestRunner.cpp TestRunner.cpp.bak
fi
if [ -f "TestRunnerMinimal.cpp" ]; then
    mv TestRunnerMinimal.cpp TestRunnerMinimal.cpp.bak
fi
if [ -f "MinimalTest.cpp" ]; then
    mv MinimalTest.cpp MinimalTest.cpp.bak
fi

# Create simple test runner for this test
cat > TestRunner.cpp << EOF
#include "TestFramework.h"

// Declare the specific test setup function
extern void $setup_func();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           TEST RUNNER WITH MONITOR");
    Serial.println("==========================================");
    Serial.println("Running: $test_name");
    Serial.println("");
    
    $setup_func();
    testRegistry.runAllTests();
    
    Serial.println("==========================================");
    Serial.println("           TEST COMPLETED");
    Serial.println("==========================================");
}

void loop() {
    delay(1000);
}
EOF

cd ..

# Update platformio_test.ini to only include our test
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

; Test environment configuration - only include our test
build_src_filter = +<*> -<../src/main.cpp> -<main.cpp> +<TestRunner.cpp> +<$test_file>
EOF

# Run the test
echo "Building and uploading test..."
pio test --environment uno_r4_wifi_test --upload-port COM5

echo ""
echo "Test uploaded successfully!"
echo "Opening serial monitor to see test output..."
echo "Press Ctrl+C to stop monitoring"
echo ""

# Open serial monitor to see test output
pio device monitor --filter time,colorize

# Restore test directory
echo "Restoring test directory..."
rm -rf test
mv test_backup_$$ test

echo ""
echo "Test completed: $test_name"
echo "----------------------------------------" 
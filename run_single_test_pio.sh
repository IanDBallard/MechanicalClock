#!/bin/bash

# Simple PIO Test Runner for Mechanical Clock Project
# Usage: ./run_single_test_pio.sh [test_number]

echo "=========================================="
echo "    SIMPLE PIO TEST RUNNER"
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

# Create a simple test runner that only includes this test
cat > test/SimpleTestRunner.cpp << EOF
#include "TestFramework.h"

// Declare the specific test setup function
extern void $setup_func();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           SIMPLE TEST RUNNER");
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

# Update the test platformio.ini to use only this test
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
build_src_filter = +<*> -<../src/main.cpp> -<main.cpp> -<TestRunner.cpp> -<TestRunnerMinimal.cpp> -<MinimalTest.cpp> +<SimpleTestRunner.cpp> +<$test_file>
EOF

echo "Test environment configured for: $test_name"
echo ""
echo "Now you can run this test from PlatformIO menu:"
echo "1. Go to PlatformIO → Build"
echo "2. Select 'uno_r4_wifi_test' environment"
echo "3. Click Build"
echo "4. Open PlatformIO → Monitor to see output"
echo ""
echo "Or run from command line:"
echo "pio run --environment uno_r4_wifi_test --target upload"
echo "pio device monitor"
echo ""
echo "----------------------------------------" 
#!/bin/bash

# Clean Single Test Runner for Mechanical Clock Project
# Usage: ./run_single_test_clean.sh [test_number]

echo "=========================================="
echo "    CLEAN SINGLE TEST RUNNER"
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

# Create temporary test directory
TEMP_TEST_DIR="test_temp_$$"
mkdir -p "$TEMP_TEST_DIR/test"

# Copy only the necessary files to test directory
cp test/TestFramework.h "$TEMP_TEST_DIR/test/"
cp test/TestFramework.cpp "$TEMP_TEST_DIR/test/"
cp test/"$test_file" "$TEMP_TEST_DIR/test/"

# Create clean test runner
cat > "$TEMP_TEST_DIR/test/TestRunner.cpp" << EOF
#include "TestFramework.h"

// Declare the specific test setup function
extern void $setup_func();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           CLEAN TEST RUNNER");
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

# Create clean platformio.ini for this test
cat > "$TEMP_TEST_DIR/platformio.ini" << EOF
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

; Test environment configuration - clean build
build_src_filter = +<*> -<../src/main.cpp>
EOF

# Copy source files needed for testing
cp -r src "$TEMP_TEST_DIR/"

# Change to temp directory and run test
cd "$TEMP_TEST_DIR"
echo "Building and uploading test from clean environment..."
pio test --environment uno_r4_wifi_test --upload-port COM5

# Return to original directory
cd ..

# Clean up
rm -rf "$TEMP_TEST_DIR"

echo ""
echo "Test completed: $test_name"
echo "----------------------------------------" 
#!/bin/bash

# Basic Test Runner for Mechanical Clock Project
# Usage: ./run_test_basic.sh [test_number]

echo "=========================================="
echo "    BASIC TEST RUNNER"
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

# Just build and upload using the existing test environment
echo "Building and uploading test..."
echo "Note: This will use the existing test setup. Check serial monitor for output."
echo ""

# Run the test build and upload
pio run --environment uno_r4_wifi_test --target upload --upload-port COM5

echo ""
echo "Test uploaded: $test_name"
echo "To see test output, run: pio device monitor"
echo "----------------------------------------" 
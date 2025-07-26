#!/bin/bash

# Unit Test Runner for Arduino R4 WiFi Mechanical Clock
# This script uploads and runs the unit tests

echo "🧪 Starting Unit Tests for Mechanical Clock V2.0.0-alpha"
echo "=================================================="

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo "❌ Error: platformio.ini not found. Please run this script from the project root."
    exit 1
fi

# Check if test directory exists
if [ ! -d "test" ]; then
    echo "❌ Error: test/ directory not found."
    exit 1
fi

echo "📋 Test Framework Overview:"
echo "  - 49 total tests across 4 test suites"
echo "  - TimeUtils: 12 tests (DST, time conversions)"
echo "  - LED: 10 tests (state management, pin control)"
echo "  - NetworkManager: 15 tests (WiFi, NTP, timezone)"
echo "  - StateManager: 12 tests (state machine, transitions)"
echo ""

# Build and upload test code
echo "🚀 Building and uploading test code..."
if pio run --target upload; then
    echo "✅ Upload successful!"
else
    echo "❌ Upload failed!"
    exit 1
fi

echo ""
echo "📊 Running tests (opening serial monitor)..."
echo "Press Ctrl+C to stop monitoring"
echo ""

# Run tests and monitor output
pio device monitor --filter time,colorize

echo ""
echo "🏁 Test run completed!"
echo "Check the output above for test results." 
#!/bin/bash

echo "=== Clock Advance Test Runner ==="
echo "This test advances the clock 1 hour in 18 seconds"
echo "18 seconds real time = 1 second clock time"
echo ""

# Build the test
echo "Building Clock Advance Test..."
pio run -e uno_r4_wifi_clock_advance_test

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful! Uploading to device..."
    
    # Upload the test
    pio run -e uno_r4_wifi_clock_advance_test -t upload
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "Upload successful! Starting monitor..."
        echo "Watch the serial output for test progress."
        echo "The test will run for 18 seconds and advance the clock 1 hour."
        echo ""
        echo "Press Ctrl+C to stop monitoring when test completes."
        echo ""
        
        # Start the monitor
        pio device monitor --environment uno_r4_wifi_clock_advance_test
    else
        echo "Upload failed!"
        exit 1
    fi
else
    echo "Build failed!"
    exit 1
fi
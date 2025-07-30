#!/bin/bash

echo "=========================================="
echo "    POWER RECOVERY TEST RUNNER"
echo "=========================================="
echo ""

echo "Backing up main.cpp..."
cp src/main.cpp src/main.cpp.backup

echo "Copying power recovery test..."
cp test/PowerRecoveryTestRunner.cpp src/main.cpp

echo "Building power recovery test..."
pio run -e uno_r4_wifi_test

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed!"
    echo "Restoring original main.cpp..."
    cp src/main.cpp.backup src/main.cpp
    rm src/main.cpp.backup
    exit 1
fi

echo ""
echo "Build successful! Uploading to Arduino..."
echo ""
echo "NOTE: Make sure your Arduino is connected to COM5"
echo "If using a different port, edit this script."
echo ""

pio run -e uno_r4_wifi_test --target upload --upload-port COM5

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Upload failed!"
    echo "Check that Arduino is connected and port is correct."
    echo "Restoring original main.cpp..."
    cp src/main.cpp.backup src/main.cpp
    rm src/main.cpp.backup
    exit 1
fi

echo ""
echo "Restoring original main.cpp..."
cp src/main.cpp.backup src/main.cpp
rm src/main.cpp.backup

echo ""
echo "Upload successful! Starting monitor..."
echo ""
echo "The power recovery test will now run automatically."
echo "You should see test output in the monitor."
echo ""
echo "Press Ctrl+C to stop monitoring."
echo ""

pio device monitor --port COM5
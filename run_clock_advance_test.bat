@echo off
echo === Clock Advance Test Runner ===
echo This test advances the clock 1 hour in 18 seconds
echo 18 seconds real time = 1 second clock time
echo.

REM Build the test
echo Building Clock Advance Test...
pio run -e uno_r4_wifi_clock_advance_test

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Uploading to device...
    
    REM Upload the test
    pio run -e uno_r4_wifi_clock_advance_test -t upload
    
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Upload successful! Starting monitor...
        echo Watch the serial output for test progress.
        echo The test will run for 18 seconds and advance the clock 1 hour.
        echo.
        echo Press Ctrl+C to stop monitoring when test completes.
        echo.
        
        REM Start the monitor
        pio device monitor --environment uno_r4_wifi_clock_advance_test
    ) else (
        echo Upload failed!
        pause
        exit /b 1
    )
) else (
    echo Build failed!
    pause
    exit /b 1
)
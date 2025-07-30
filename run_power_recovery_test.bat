@echo off
echo ==========================================
echo    POWER RECOVERY TEST RUNNER
echo ==========================================
echo.

echo Backing up main.cpp...
copy src\main.cpp src\main.cpp.backup >nul

echo Copying power recovery test...
copy test\PowerRecoveryTestRunner.cpp src\main.cpp >nul

echo Building power recovery test...
pio run -e uno_r4_wifi_test

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    echo Restoring original main.cpp...
    copy src\main.cpp.backup src\main.cpp >nul
    del src\main.cpp.backup >nul
    pause
    exit /b 1
)

echo.
echo Build successful! Uploading to Arduino...
echo.
echo NOTE: Make sure your Arduino is connected to COM5
echo If using a different port, edit this script.
echo.

pio run -e uno_r4_wifi_test --target upload --upload-port COM5

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Upload failed!
    echo Check that Arduino is connected and port is correct.
    echo Restoring original main.cpp...
    copy src\main.cpp.backup src\main.cpp >nul
    del src\main.cpp.backup >nul
    pause
    exit /b 1
)

echo.
echo Restoring original main.cpp...
copy src\main.cpp.backup src\main.cpp >nul
del src\main.cpp.backup >nul

echo.
echo Upload successful! Starting monitor...
echo.
echo The power recovery test will now run automatically.
echo You should see test output in the monitor.
echo.
echo Press Ctrl+C to stop monitoring.
echo.

pio device monitor --port COM5

pause
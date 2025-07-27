@echo off
echo Building Desktop Unit Tests for Mechanical Clock...
echo ================================================

REM Check if g++ is available
g++ --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: g++ not found. Please install MinGW-w64 or add it to your PATH.
    echo You can install it via: winget install --id=MSYS2.MSYS2
    pause
    exit /b 1
)

REM Compile the test program
echo Compiling test program...
g++ -std=c++17 -I../src -o mechanical_clock_tests.exe run_tests_desktop.cpp ../src/TimeUtils.cpp ../src/LED.cpp

if errorlevel 1 (
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo.
echo Compilation successful! Running tests...
echo ================================================

REM Run the tests
mechanical_clock_tests.exe

echo.
echo Tests completed!
pause 
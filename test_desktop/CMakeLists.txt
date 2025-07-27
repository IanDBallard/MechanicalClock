cmake_minimum_required(VERSION 3.10)
project(MechanicalClockTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test_desktop
)

# Mock Arduino environment
add_definitions(-DARDUINO_TESTING=1)
add_definitions(-DTEST_VERBOSE=1)

# Source files for testing
set(TEST_SOURCES
    test_desktop/main_test.cpp
    test_desktop/TestFramework.cpp
    test_desktop/TimeUtilsTest.cpp
    test_desktop/LEDTest.cpp
    test_desktop/NetworkManagerTest.cpp
    test_desktop/StateManagerTest.cpp
)

# Source files for the classes being tested (with mocks)
set(CLASS_SOURCES
    src/TimeUtils.cpp
    src/LED.cpp
    src/NetworkManager.cpp
    src/StateManager.cpp
    src/LCDDisplay.cpp
    src/MechanicalClock.cpp
    src/DigitalClock.cpp
)

# Create test executable
add_executable(mechanical_clock_tests
    ${TEST_SOURCES}
    ${CLASS_SOURCES}
)

# Link libraries
target_link_libraries(mechanical_clock_tests
    GTest::gtest
    GTest::gtest_main
    pthread
)

# Enable testing
enable_testing()
add_test(NAME MechanicalClockTests COMMAND mechanical_clock_tests) 
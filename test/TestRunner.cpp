#include "TestFramework.h"
#include "TimeUtilsTest.cpp"
#include "LEDTest.cpp"
#include "NetworkManagerTest.cpp"
#include "StateManagerTest.cpp"

// Function to set up all test suites
void setupTests() {
    Serial.println("Setting up test suites...");
    
    // Set up individual test suites
    setupTimeUtilsTests();
    setupLEDTests();
    setupNetworkManagerTests();
    setupStateManagerTests();
    
    Serial.println("All test suites configured.");
}

// Main test setup and loop
TEST_SETUP();

TEST_LOOP(); 
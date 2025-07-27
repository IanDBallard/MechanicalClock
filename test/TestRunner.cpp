#include "TestFramework.h"

// Declare setup functions (defined in other test files)
extern void setupTimeUtilsTests();
extern void setupLEDTests();
extern void setupNetworkManagerTests();
extern void setupStateManagerTests();
extern void setupPowerUpTests();
extern void setupPowerOffRecoveryTests();

// Global test registry
TestRegistry testRegistry;

// Function to set up all test suites
void setupTests() {
    Serial.println("Setting up test suites...");
    
    // Set up individual test suites
    setupTimeUtilsTests();
    setupLEDTests();
    setupNetworkManagerTests();
    setupStateManagerTests();
    setupPowerUpTests();
    setupPowerOffRecoveryTests();
    
    Serial.println("All test suites configured.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           ARDUINO UNIT TESTS");
    Serial.println("==========================================");
    Serial.println();
    
    setupTests();
    testRegistry.runAllTests();
}

void loop() {
    // Tests run once in setup()
    delay(1000);
} 
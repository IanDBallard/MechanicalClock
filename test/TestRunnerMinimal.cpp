#include "TestFramework.h"

// Declare minimal test setup function
extern void setupMinimalTests();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.println("DEBUG: setup() - Starting minimal Arduino setup");
    Serial.begin(115200);
    Serial.println("DEBUG: setup() - Serial initialized");
    
    delay(1000);
    Serial.println("DEBUG: setup() - Delay completed");
    
    Serial.println("==========================================");
    Serial.println("           MINIMAL UNIT TEST");
    Serial.println("==========================================");
    Serial.println();
    
    Serial.println("DEBUG: setup() - About to call setupMinimalTests()");
    setupMinimalTests();
    Serial.println("DEBUG: setup() - setupMinimalTests() completed");
    
    Serial.println("DEBUG: setup() - About to call testRegistry.runAllTests()");
    testRegistry.runAllTests();
    Serial.println("DEBUG: setup() - testRegistry.runAllTests() completed");
    
    Serial.println("DEBUG: setup() - Setup function completed successfully");
}

void loop() {
    Serial.println("DEBUG: loop() - Entering main loop");
    delay(1000);
    Serial.println("DEBUG: loop() - Loop iteration completed");
} 
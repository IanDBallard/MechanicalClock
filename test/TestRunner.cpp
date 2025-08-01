#include "TestFramework.h"

// Declare the specific test setup function
extern void setupTimeUtilsTests();

// Global test registry
TestRegistry testRegistry;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("==========================================");
    Serial.println("           SIMPLE TEST RUNNER");
    Serial.println("==========================================");
    Serial.println("Running: TimeUtilsTest");
    Serial.println("");
    
    setupTimeUtilsTests();
    testRegistry.runAllTests();
    
    Serial.println("==========================================");
    Serial.println("           TEST COMPLETED");
    Serial.println("==========================================");
}

void loop() {
    delay(1000);
}

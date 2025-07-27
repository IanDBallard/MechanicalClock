#include "TestFramework.h"

// Minimal test suite
TestSuite testSuite_MinimalTest("MinimalTest");

// Simple test function
void testBasicFunctionality() {
    Serial.println("DEBUG: testBasicFunctionality() - Starting basic test");
    
    // Simple assertion
    ASSERT_TRUE(true);
    
    Serial.println("DEBUG: testBasicFunctionality() - Basic test passed");
    Serial.println("✓ Basic functionality test passed");
}

void setupMinimalTests() {
    Serial.println("DEBUG: setupMinimalTests() - Starting minimal test setup");
    
    testSuite_MinimalTest.addTest("testBasicFunctionality", testBasicFunctionality);
    
    // Register with global test registry
    extern TestRegistry testRegistry;
    testRegistry.addSuite(testSuite_MinimalTest);
    
    Serial.println("DEBUG: setupMinimalTests() - Minimal test setup completed");
} 
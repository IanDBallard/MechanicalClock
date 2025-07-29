#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <WiFiS3.h>
#include <RTC.h>

// Test result structure
struct TestResult {
    String testName;
    bool passed;
    String message;
    unsigned long duration;
    
    TestResult(const String& name, bool pass, const String& msg = "", unsigned long dur = 0)
        : testName(name), passed(pass), message(msg), duration(dur) {}
};

// Test case structure
struct TestCase {
    String name;
    std::function<void()> testFunction;
    bool shouldRun;
    
    TestCase(const String& testName, std::function<void()> func, bool run = true)
        : name(testName), testFunction(func), shouldRun(run) {}
};

// Test suite structure
struct TestSuite {
    String name;
    std::vector<TestCase> tests;
    unsigned long totalDuration;
    int passedTests;
    int failedTests;
    
    TestSuite(const String& suiteName) 
        : name(suiteName), totalDuration(0), passedTests(0), failedTests(0) {}
    
    void addTest(const String& testName, std::function<void()> testFunc, bool shouldRun = true) {
        tests.push_back(TestCase(testName, testFunc, shouldRun));
    }
    
    void run() {
        Serial.print("DEBUG: TestSuite::run() - Starting suite: ");
        Serial.println(name);
        Serial.println("=== Running Test Suite: " + name + " ===");
        totalDuration = millis();
        passedTests = 0;
        failedTests = 0;
        
        Serial.print("DEBUG: TestSuite::run() - Number of tests in suite: ");
        Serial.println(tests.size());
        
        for (auto& test : tests) {
            Serial.print("DEBUG: TestSuite::run() - About to run test: ");
            Serial.println(test.name);
            
            if (!test.shouldRun) {
                Serial.println("Skipping: " + test.name);
                continue;
            }
            
            unsigned long startTime = millis();
            bool testPassed = true;
            String errorMessage = "";
            
            Serial.print("DEBUG: TestSuite::run() - Executing test: ");
            Serial.println(test.name);
            
            try {
                test.testFunction();
                Serial.print("DEBUG: TestSuite::run() - Test completed without exception: ");
                Serial.println(test.name);
            } catch (const String& e) {
                testPassed = false;
                errorMessage = e;
                Serial.print("DEBUG: TestSuite::run() - Test threw String exception: ");
                Serial.println(test.name);
            } catch (...) {
                testPassed = false;
                errorMessage = "Unknown exception";
                Serial.print("DEBUG: TestSuite::run() - Test threw unknown exception: ");
                Serial.println(test.name);
            }
            
            unsigned long endTime = millis();
            unsigned long duration = endTime - startTime;
            
            if (testPassed) {
                passedTests++;
                Serial.println("✓ PASS: " + test.name + " (" + String(duration) + "ms)");
            } else {
                failedTests++;
                Serial.println("✗ FAIL: " + test.name + " (" + String(duration) + "ms)");
                if (errorMessage.length() > 0) {
                    Serial.println("  Error: " + errorMessage);
                }
            }
            
            Serial.print("DEBUG: TestSuite::run() - Completed test: ");
            Serial.println(test.name);
        }
        
        totalDuration = millis() - totalDuration;
        Serial.println("=== Test Suite Results: " + name + " ===");
        Serial.println("Passed: " + String(passedTests) + ", Failed: " + String(failedTests));
        Serial.println("Total Duration: " + String(totalDuration) + "ms");
        Serial.println();
        
        Serial.print("DEBUG: TestSuite::run() - Completed suite: ");
        Serial.println(name);
    }
};

// Global test registry
class TestRegistry {
private:
    std::vector<TestSuite> suites;
    
public:
    void addSuite(const TestSuite& suite) {
        Serial.print("DEBUG: TestRegistry::addSuite() - Adding suite: ");
        Serial.println(suite.name);
        suites.push_back(suite);
        Serial.print("DEBUG: TestRegistry::addSuite() - Total suites now: ");
        Serial.println(suites.size());
    }
    
    void runAllTests() {
        Serial.println("DEBUG: TestRegistry::runAllTests() - Starting test execution");
        Serial.println("==========================================");
        Serial.println("           UNIT TEST FRAMEWORK");
        Serial.println("==========================================");
        Serial.println();
        
        unsigned long totalStartTime = millis();
        Serial.println("DEBUG: TestRegistry::runAllTests() - Total start time recorded");
        int totalPassed = 0;
        int totalFailed = 0;
        
        Serial.print("DEBUG: TestRegistry::runAllTests() - Number of test suites: ");
        Serial.println(suites.size());
        
        for (auto& suite : suites) {
            Serial.print("DEBUG: TestRegistry::runAllTests() - About to run suite: ");
            Serial.println(suite.name);
            suite.run();
            Serial.print("DEBUG: TestRegistry::runAllTests() - Completed suite: ");
            Serial.println(suite.name);
            totalPassed += suite.passedTests;
            totalFailed += suite.failedTests;
        }
        
        unsigned long totalEndTime = millis();
        unsigned long totalDuration = totalEndTime - totalStartTime;
        
        Serial.println("DEBUG: TestRegistry::runAllTests() - All suites completed, generating summary");
        Serial.println("==========================================");
        Serial.println("           FINAL RESULTS");
        Serial.println("==========================================");
        Serial.println("Total Tests Passed: " + String(totalPassed));
        Serial.println("Total Tests Failed: " + String(totalFailed));
        Serial.println("Total Duration: " + String(totalDuration) + "ms");
        Serial.println("==========================================");
        Serial.println("DEBUG: TestRegistry::runAllTests() - Test execution completed");
    }
    
    void runSuite(const String& suiteName) {
        for (auto& suite : suites) {
            if (suite.name == suiteName) {
                suite.run();
                return;
            }
        }
        Serial.println("Test suite '" + suiteName + "' not found!");
    }
};

// Global test registry instance
extern TestRegistry testRegistry;

// Test registration macros
#define TEST_SUITE(name) TestSuite testSuite_##name(#name)
#define ADD_TEST(suite, name) suite.addTest(#name, [&]() -> void
#define END_TEST );

// Assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw String("ASSERT_TRUE failed: " #condition " at line ") + String(__LINE__); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw String("ASSERT_FALSE failed: " #condition " at line ") + String(__LINE__); \
    }

#define ASSERT_EQUAL(expected, actual) \
    if ((expected) != (actual)) { \
        throw String("ASSERT_EQUAL failed: expected ") + String(expected) + \
              ", got " + String(actual) + " at line " + String(__LINE__); \
    }

#define ASSERT_NOT_EQUAL(expected, actual) \
    if ((expected) == (actual)) { \
        throw String("ASSERT_NOT_EQUAL failed: both values are ") + String(expected) + \
              " at line " + String(__LINE__); \
    }

#define ASSERT_NULL(ptr) \
    if ((ptr) != nullptr) { \
        throw String("ASSERT_NULL failed: pointer is not null at line ") + String(__LINE__); \
    }

#define ASSERT_NOT_NULL(ptr) \
    if ((ptr) == nullptr) { \
        throw String("ASSERT_NOT_NULL failed: pointer is null at line ") + String(__LINE__); \
    }

#define ASSERT_STRING_EQUAL(expected, actual) \
    if (String(expected) != String(actual)) { \
        throw String("ASSERT_STRING_EQUAL failed: expected '") + String(expected) + \
              "', got '" + String(actual) + "' at line " + String(__LINE__); \
    }

#define ASSERT_STRING_CONTAINS(haystack, needle) \
    if (String(haystack).indexOf(String(needle)) == -1) { \
        throw String("ASSERT_STRING_CONTAINS failed: '") + String(haystack) + \
              "' does not contain '" + String(needle) + "' at line " + String(__LINE__); \
    }

#define ASSERT_GREATER_THAN(value1, value2) \
    if ((value1) <= (value2)) { \
        throw String("ASSERT_GREATER_THAN failed: ") + String(value1) + \
              " is not greater than " + String(value2) + " at line " + String(__LINE__); \
    }

#define ASSERT_LESS_THAN(value1, value2) \
    if ((value1) >= (value2)) { \
        throw String("ASSERT_LESS_THAN failed: ") + String(value1) + \
              " is not less than " + String(value2) + " at line " + String(__LINE__); \
    }

#define ASSERT_IN_RANGE(value, min, max) \
    if ((value) < (min) || (value) > (max)) { \
        throw String("ASSERT_IN_RANGE failed: ") + String(value) + \
              " is not in range [" + String(min) + ", " + String(max) + "] at line " + String(__LINE__); \
    }

// Utility macros for testing
#define TEST_SETUP() \
    void setup() { \
        Serial.begin(115200); \
        delay(1000); /* Give Serial time to initialize without blocking */ \
        Serial.println("Starting unit tests..."); \
        setupTests(); \
        testRegistry.runAllTests(); \
    }

#define TEST_LOOP() \
    void loop() { \
        // Tests run once in setup() \
        delay(1000); \
    }

// Mock objects for testing
class MockWiFi {
public:
    static int status() { return mockWiFiStatus; }
    static void setStatus(int newStatus) { mockWiFiStatus = newStatus; }
    static void reset() { mockWiFiStatus = WL_DISCONNECTED; }
    
private:
    static int mockWiFiStatus;
};

class MockRTC {
public:
    static RTCTime getTime() { return mockCurrentTime; }
    static void setTime(const RTCTime& time) { mockCurrentTime = time; }
    static void reset() { mockCurrentTime = RTCTime(); }
    
private:
    static RTCTime mockCurrentTime;
};

// Test utilities
class TestUtils {
public:
    static void delay(unsigned long ms) {
        // In test environment, we might want to skip actual delays
        #ifdef ARDUINO_TESTING
        // Skip delays during testing
        #else
        ::delay(ms);
        #endif
    }
    
    static unsigned long millis() {
        return ::millis();
    }
    
    static void resetMocks() {
        MockWiFi::reset();
        MockRTC::reset();
    }
};

#endif // TEST_FRAMEWORK_H 
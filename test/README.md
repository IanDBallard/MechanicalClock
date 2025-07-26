# Unit Testing Framework for Arduino R4 WiFi Mechanical Clock

This directory contains a comprehensive unit testing framework for the mechanical clock project. The framework is designed to run on the Arduino R4 WiFi itself, providing real-time testing of all components.

## 🏗️ Framework Architecture

### Core Components

- **`TestFramework.h/cpp`** - Main testing framework with test runner, assertions, and mock objects
- **`TestRunner.cpp`** - Main entry point that sets up and runs all test suites
- **Individual Test Files** - Separate test suites for each class:
  - `TimeUtilsTest.cpp` - Tests for time utilities and DST calculations
  - `LEDTest.cpp` - Tests for LED control class
  - `NetworkManagerTest.cpp` - Tests for WiFi and NTP functionality
  - `StateManagerTest.cpp` - Tests for state machine logic

## 🚀 Getting Started

### Running Tests on Arduino R4 WiFi

1. **Upload Test Code:**
   ```bash
   # Copy test files to your Arduino sketch
   cp test/*.cpp test/*.h src/
   
   # Upload to Arduino R4 WiFi
   pio run --target upload
   ```

2. **Monitor Test Results:**
   ```bash
   # Open serial monitor to see test output
   pio device monitor
   ```

3. **Expected Output:**
   ```
   ==========================================
              UNIT TEST FRAMEWORK
   ==========================================
   
   === Running Test Suite: TimeUtilsTest ===
   ✓ PASS: MonthNamesArray (2ms)
   ✓ PASS: DayOfWeekAbbreviations (1ms)
   ✓ PASS: Month2intFunction (1ms)
   ...
   
   === Test Suite Results: TimeUtilsTest ===
   Passed: 12, Failed: 0
   Total Duration: 45ms
   
   ==========================================
              FINAL RESULTS
   ==========================================
   Total Tests Passed: 45
   Total Tests Failed: 0
   Total Duration: 234ms
   🎉 ALL TESTS PASSED! 🎉
   ==========================================
   ```

## 📋 Available Assertions

The framework provides comprehensive assertion macros:

### Basic Assertions
```cpp
ASSERT_TRUE(condition);           // Assert condition is true
ASSERT_FALSE(condition);          // Assert condition is false
ASSERT_EQUAL(expected, actual);   // Assert values are equal
ASSERT_NOT_EQUAL(expected, actual); // Assert values are not equal
```

### Pointer Assertions
```cpp
ASSERT_NULL(ptr);                 // Assert pointer is null
ASSERT_NOT_NULL(ptr);             // Assert pointer is not null
```

### String Assertions
```cpp
ASSERT_STRING_EQUAL(expected, actual);     // Assert strings are equal
ASSERT_STRING_CONTAINS(haystack, needle);  // Assert string contains substring
```

### Numeric Assertions
```cpp
ASSERT_GREATER_THAN(value1, value2);       // Assert value1 > value2
ASSERT_LESS_THAN(value1, value2);          // Assert value1 < value2
ASSERT_IN_RANGE(value, min, max);          // Assert value is in range
```

## 🧪 Writing New Tests

### Creating a Test Suite

1. **Create Test File:**
   ```cpp
   #include "TestFramework.h"
   #include "../src/YourClass.h"
   
   TEST_SUITE(YourClassTest);
   
   void setupYourClassTests() {
       // Test constructor
       ADD_TEST(testSuite_YourClassTest, Constructor) {
           YourClass obj(42);
           ASSERT_EQUAL(42, obj.getValue());
       } END_TEST;
       
       // Test methods
       ADD_TEST(testSuite_YourClassTest, MethodTest) {
           YourClass obj(10);
           obj.doSomething();
           ASSERT_TRUE(obj.isValid());
       } END_TEST;
       
       testRegistry.addSuite(testSuite_YourClassTest);
   }
   ```

2. **Add to Test Runner:**
   ```cpp
   // In TestRunner.cpp
   #include "YourClassTest.cpp"
   
   void setupTests() {
       // ... existing setup ...
       setupYourClassTests();
   }
   ```

### Test Structure

Each test follows this pattern:
```cpp
ADD_TEST(testSuite_Name, TestName) {
    // Setup
    YourClass obj(42);
    
    // Exercise
    obj.doSomething();
    
    // Verify
    ASSERT_EQUAL(expected, obj.getResult());
} END_TEST;
```

## 🔧 Mock Objects

The framework includes mock objects for testing:

### MockWiFi
```cpp
MockWiFi::setStatus(WL_CONNECTED);    // Mock WiFi connected
MockWiFi::setStatus(WL_DISCONNECTED); // Mock WiFi disconnected
MockWiFi::reset();                    // Reset to default state
```

### MockRTC
```cpp
RTCTime testTime(15, Month::JANUARY, 2024, 12, 0, 0, DayOfWeek::MONDAY);
MockRTC::setTime(testTime);           // Mock RTC time
MockRTC::reset();                     // Reset to default state
```

## 📊 Test Coverage

### Current Test Coverage

| Class | Test Count | Coverage Areas |
|-------|------------|----------------|
| **TimeUtils** | 12 tests | DST calculations, month/day conversions, edge cases |
| **LED** | 10 tests | State management, pin control, multiple LEDs |
| **NetworkManager** | 15 tests | WiFi status, NTP sync, timezone handling |
| **StateManager** | 12 tests | State transitions, error handling, state logic |

### Test Categories

- **Unit Tests** - Individual class functionality
- **Integration Tests** - Class interactions
- **Edge Case Tests** - Boundary conditions and error scenarios
- **State Tests** - State machine transitions and logic

## 🛠️ Configuration

### Test Environment

The framework automatically detects the test environment:

```cpp
#ifdef ARDUINO_TESTING
// Test-specific behavior (e.g., skip delays)
#else
// Normal Arduino behavior
#endif
```

### Customizing Test Behavior

```cpp
// Skip specific tests
ADD_TEST(testSuite_Name, TestName, false) { // false = skip test
    // Test code
} END_TEST;

// Custom test timeouts
unsigned long startTime = millis();
while (condition && (millis() - startTime < 5000)) {
    // Test with timeout
}
```

## 🔍 Debugging Tests

### Verbose Output

Enable detailed test output:
```cpp
#define TEST_VERBOSE 1
```

### Test Isolation

Each test runs in isolation with fresh mocks:
```cpp
void setupTests() {
    TestUtils::resetMocks(); // Reset all mocks before tests
    // ... test setup
}
```

### Error Reporting

Tests provide detailed error messages:
```
✗ FAIL: DSTCalculationWinter (5ms)
  Error: ASSERT_FALSE failed: isDST at line 45
```

## 📈 Continuous Integration

### Automated Testing

The framework supports automated testing workflows:

1. **Upload and Run:**
   ```bash
   pio run --target upload && pio device monitor --filter time
   ```

2. **Parse Results:**
   ```bash
   # Extract test results
   pio device monitor | grep -E "(PASS|FAIL|Total Tests)"
   ```

3. **Exit Codes:**
   - All tests pass: Exit code 0
   - Any test fails: Exit code 1

## 🎯 Best Practices

### Test Organization
- Group related tests in the same suite
- Use descriptive test names
- Test one concept per test
- Follow AAA pattern (Arrange, Act, Assert)

### Test Data
- Use realistic test data
- Test edge cases and boundary conditions
- Include both positive and negative test cases

### Mock Usage
- Mock external dependencies
- Reset mocks between tests
- Verify mock interactions when needed

### Performance
- Keep tests fast (avoid long delays)
- Use appropriate timeouts
- Test in isolation to avoid interference

## 🚨 Troubleshooting

### Common Issues

1. **Tests Not Running:**
   - Check serial monitor is open
   - Verify test files are included in build
   - Check for compilation errors

2. **Mock Objects Not Working:**
   - Ensure mocks are reset between tests
   - Check mock state is properly set
   - Verify mock functions are called

3. **Test Failures:**
   - Check assertion messages for details
   - Verify test data is correct
   - Check for timing issues

### Debug Commands

```cpp
// Add debug output to tests
Serial.println("Debug: " + String(value));

// Check mock state
Serial.println("Mock WiFi Status: " + String(MockWiFi::status()));
```

## 📚 Additional Resources

- [Arduino Testing Best Practices](https://docs.arduino.cc/learn/programming/testing)
- [Unit Testing Patterns](https://martinfowler.com/bliki/UnitTest.html)
- [Mock Object Patterns](https://martinfowler.com/articles/mocksArentStubs.html)

---

**Happy Testing! 🧪✨** 
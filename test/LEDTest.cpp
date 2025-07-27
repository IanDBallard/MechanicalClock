#include "TestFramework.h"
#include "../src/LED.h"

TEST_SUITE(LEDTest);

// Mock pin state for testing
static int mockPinState = LOW;
static int mockPinNumber = -1;

// Mock digitalWrite function for testing
void mockDigitalWrite(int pin, int state) {
    mockPinNumber = pin;
    mockPinState = state;
}

// Mock pinMode function for testing
void mockPinMode(int pin, int mode) {
    mockPinNumber = pin;
}

void setupLEDTests() {
    // Test LED constructor
    ADD_TEST(testSuite_LEDTest, LEDConstructor) {
        LED led(13);
        // Note: LED class doesn't have getPin() method, using isOn() instead
        ASSERT_FALSE(led.isOn());
        ASSERT_FALSE(led.isOn());
    } END_TEST;

    // Test LED initialization
    ADD_TEST(testSuite_LEDTest, LEDInitialization) {
        LED led(13);
        led.begin();
        // Note: In real hardware, we'd verify pinMode was called
        ASSERT_FALSE(led.isOn()); // Should start off after initialization
    } END_TEST;

    // Test LED turn on
    ADD_TEST(testSuite_LEDTest, LEDTurnOn) {
        LED led(13);
        led.begin();
        led.on();
        ASSERT_TRUE(led.isOn());
    } END_TEST;

    // Test LED turn off
    ADD_TEST(testSuite_LEDTest, LEDTurnOff) {
        LED led(13);
        led.begin();
        led.on();
        led.off();
        ASSERT_FALSE(led.isOn());
    } END_TEST;

    // Test LED toggle
    ADD_TEST(testSuite_LEDTest, LEDToggle) {
        LED led(13);
        led.begin();
        
        // Start off
        ASSERT_FALSE(led.isOn());
        
        // Toggle on
        led.toggle();
        ASSERT_TRUE(led.isOn());
        
        // Toggle off
        led.toggle();
        ASSERT_FALSE(led.isOn());
        
        // Toggle on again
        led.toggle();
        ASSERT_TRUE(led.isOn());
    } END_TEST;

    // Test LED state persistence
    ADD_TEST(testSuite_LEDTest, LEDStatePersistence) {
        LED led(13);
        led.begin();
        
        // Turn on and verify state
        led.on();
        ASSERT_TRUE(led.isOn());
        
        // Create another LED instance on same pin (simulating power cycle)
        LED led2(13);
        led2.begin();
        
        // State should be independent
        ASSERT_FALSE(led2.isOn());
    } END_TEST;

    // Test multiple LEDs
    ADD_TEST(testSuite_LEDTest, MultipleLEDs) {
        LED led1(13);
        LED led2(14);
        
        led1.begin();
        led2.begin();
        
        // Turn on first LED
        led1.on();
        ASSERT_TRUE(led1.isOn());
        ASSERT_FALSE(led2.isOn());
        
        // Turn on second LED
        led2.on();
        ASSERT_TRUE(led1.isOn());
        ASSERT_TRUE(led2.isOn());
        
        // Turn off first LED
        led1.off();
        ASSERT_FALSE(led1.isOn());
        ASSERT_TRUE(led2.isOn());
    } END_TEST;

    // Test LED with different pin numbers
    ADD_TEST(testSuite_LEDTest, LEDDifferentPins) {
        LED led1(2);
        LED led2(3);
        LED led3(4);
        
        led1.begin();
        led2.begin();
        led3.begin();
        
        // Note: LED class doesn't have getPin() method, testing state instead
        ASSERT_FALSE(led1.isOn());
        ASSERT_FALSE(led2.isOn());
        ASSERT_FALSE(led3.isOn());
        
        // Test independent control
        led1.on();
        led2.on();
        led3.off();
        
        ASSERT_TRUE(led1.isOn());
        ASSERT_TRUE(led2.isOn());
        ASSERT_FALSE(led3.isOn());
    } END_TEST;

    // Test LED state after initialization
    ADD_TEST(testSuite_LEDTest, LEDStateAfterInit) {
        LED led(13);
        ASSERT_FALSE(led.isOn()); // Should start off
        
        led.begin();
        ASSERT_FALSE(led.isOn()); // Should still be off after init
        
        led.on();
        ASSERT_TRUE(led.isOn()); // Should be on after explicit turn on
    } END_TEST;

    // Test LED edge cases
    ADD_TEST(testSuite_LEDTest, LEDEdgeCases) {
        // Test with pin 0 (valid but unusual)
        LED led1(0);
        led1.begin();
        // Note: LED class doesn't have getPin() method, testing state instead
        ASSERT_FALSE(led1.isOn());
        
        // Test with high pin number
        LED led2(255);
        led2.begin();
        // Note: LED class doesn't have getPin() method, testing state instead
        ASSERT_FALSE(led2.isOn());
        
        // Test multiple toggles
        LED led3(13);
        led3.begin();
        
        for (int i = 0; i < 10; i++) {
            bool expectedState = (i % 2 == 1);
            led3.toggle();
            ASSERT_EQUAL(expectedState, led3.isOn());
        }
    } END_TEST;

    testRegistry.addSuite(testSuite_LEDTest);
} 
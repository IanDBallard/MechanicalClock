#include "TestFramework.h"
#include "Clock.h"
#include "DigitalClock.h"
#include "MechanicalClock.h"
#include "LCDDisplay.h"
#include "NetworkManager.h"
#include "StateManager.h"
#include "TimeUtils.h"
#include "LED.h"
#include "Constants.h"

// Global instances for testing
RTClock rtcInstance;
LCDDisplay lcdDisplay;
NetworkManager networkManager(AP_SSID, IPAddress(192, 168, 1, 1), 80, 30000, 3, -5, true);
MechanicalClock mechanicalClock(
    STEP_PIN, DIR_PIN, ENABLE_PIN, MS1_PIN, MS2_PIN, MS3_PIN, LED_PIN, rtcInstance, lcdDisplay
);
StateManager stateManager(networkManager, lcdDisplay, mechanicalClock, rtcInstance);

// Test configuration
const unsigned long TEST_DURATION_MS = 18000;  // 18 seconds real time
const unsigned long CLOCK_ADVANCE_MS = 3600000; // 1 hour clock time
const unsigned long UPDATE_INTERVAL_MS = 1000;  // Update every 1 second

unsigned long testStartTime = 0;
unsigned long lastUpdateTime = 0;
time_t initialTime = 0;
time_t currentTestTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Clock Advance Test ===");
    Serial.println("Advances clock 1 hour in 18 seconds");
    Serial.println("18 seconds real time = 1 second clock time");
    Serial.println();

    // Initialize components
    rtcInstance.begin();
    lcdDisplay.begin();
    networkManager.begin();
    mechanicalClock.begin();

    // Set initial time to a known value (e.g., 12:00:00)
    struct tm t = {0};
    t.tm_year = 2024 - 1900;
    t.tm_mon = 0; // January
    t.tm_mday = 1;
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = 0;
    initialTime = mktime(&t);
    currentTestTime = initialTime;

    RTCTime initialRTC(initialTime);
    rtcInstance.setTime(initialRTC);

    Serial.print("Initial time: ");
    Serial.println(initialTime);
    Serial.print("Target time: ");
    Serial.println(initialTime + CLOCK_ADVANCE_MS / 1000);
    Serial.println();

    testStartTime = millis();
    lastUpdateTime = testStartTime;

    Serial.println("Test starting...");
    Serial.println("Time format: [Real Time] -> [Clock Time] -> [Display]");
    Serial.println();
}

void loop() {
    unsigned long currentRealTime = millis();
    unsigned long elapsedRealTime = currentRealTime - testStartTime;

    // Check if test is complete
    if (elapsedRealTime >= TEST_DURATION_MS) {
        Serial.println();
        Serial.println("=== Test Complete ===");
        Serial.print("Final clock time: ");
        Serial.println(currentTestTime);
        Serial.print("Expected final time: ");
        Serial.println(initialTime + CLOCK_ADVANCE_MS / 1000);
        Serial.println("Test finished.");

        while (true) {
            delay(1000);
        }
    }

    // Update every second
    if (currentRealTime - lastUpdateTime >= UPDATE_INTERVAL_MS) {
        lastUpdateTime = currentRealTime;

        // Calculate how much clock time should advance
        unsigned long clockAdvanceSeconds = elapsedRealTime / 18; // Compress 18 seconds to 1 second
        time_t newTestTime = initialTime + clockAdvanceSeconds;

        if (newTestTime != currentTestTime) {
            currentTestTime = newTestTime;

            // Convert to RTC time for display
            RTCTime currentRTC(currentTestTime);

            // Update the RTC with the new time
            rtcInstance.setTime(currentRTC);

            // Update display
            lcdDisplay.updateTimeAndDate(currentRTC);

            // Print progress
            Serial.print("[");
            Serial.print(elapsedRealTime / 1000);
            Serial.print("s] -> [");
            Serial.print(currentTestTime);
            Serial.print("] -> [");

            // Format time for display
            char timeStr[20];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                    currentRTC.getHour(),
                    currentRTC.getMinutes(),
                    currentRTC.getSeconds());
            Serial.print(timeStr);
            Serial.println("]");
        }
    }

    // Small delay to prevent overwhelming the system
    delay(100);
}
#include <Arduino.h>
#include "../TestFramework.h"
#include "../Clock.h"
#include "../DigitalClock.h"
#include "../MechanicalClock.h"
#include "../LCDDisplay.h"
#include "../NetworkManager.h"
#include "../StateManager.h"
#include "../TimeUtils.h"
#include "../LED.h"
#include "../Constants.h"

// Global instances for testing
RTClock RTC;
LCDDisplay lcdDisplay;
NetworkManager networkManager(AP_SSID, IPAddress(192, 168, 1, 1), 80, 30000, 3, -5, true);
MechanicalClock mechanicalClock(RTC, lcdDisplay, MECHANICAL_CLOCK_MICROSTEP_MODE);
StateManager stateManager(networkManager, lcdDisplay, mechanicalClock, RTC);

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
    RTC.begin();
    lcdDisplay.begin();
    networkManager.begin();
    mechanicalClock.begin();
    
    // Set initial time to a known value (e.g., 12:00:00)
    RTCTime initialRTC(2024, 1, 1, 12, 0, 0, 0); // Monday, Jan 1, 2024, 12:00:00
    RTC.setTime(initialRTC);
    initialTime = initialRTC.getUnixTime();
    currentTestTime = initialTime;
    
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
        // 18 seconds real time = 1 second clock time
        // So 1 second real time = 1/18 second clock time
        unsigned long clockAdvanceSeconds = elapsedRealTime / 18; // Compress 18 seconds to 1 second
        time_t newTestTime = initialTime + clockAdvanceSeconds;
        
        if (newTestTime != currentTestTime) {
            currentTestTime = newTestTime;
            
            // Convert to RTC time for display
            RTCTime currentRTC;
            currentRTC.setUnixTime(currentTestTime);
            
            // Update the RTC with the new time
            RTC.setTime(currentRTC);
            
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
                    currentRTC.getHours(), 
                    currentRTC.getMinutes(), 
                    currentRTC.getSeconds());
            Serial.print(timeStr);
            Serial.println("]");
        }
    }
    
    // Small delay to prevent overwhelming the system
    delay(100);
}
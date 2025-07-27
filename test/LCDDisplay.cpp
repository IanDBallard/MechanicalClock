#include "LCDDisplay.h"
#include <Arduino.h> // For millis(), Serial.println()
#include <stdio.h>   // For snprintf
#include "TimeUtils.h" // Include TimeUtils for utility functions and arrays

// LCDDisplay class implementation
// The extern declarations for Month2int, DayOfWeek2int, MONTH_NAMES, DOW_ABBREV
// are implicitly handled by including "TimeUtils.h" which contains their declarations.

// Constructor: Initializes the LiquidCrystal_I2C object with the given address
LCDDisplay::LCDDisplay(uint8_t address)
    : _lcd(address, 16, 2), _initialized(false), _address(address) {
    // Member initializers list is used for _lcd, _initialized, _address.
    // Other members (_lastDisplayedSecond, etc.) are initialized to -1 by default.
}

// begin(): Initializes the LCD hardware and checks for its presence
bool LCDDisplay::begin() {
    Serial.println("LCDDisplay::begin() called.");
    Wire.begin(); // Initialize I2C communication

    // Attempt to detect LCD at the provided address
    byte error;
    Wire.beginTransmission(_address);
    error = Wire.endTransmission();
    
    // If default address fails, try the common alternative 0x3F
    if (error != 0) {
        Serial.print("LCD not found at 0x"); Serial.print(_address, HEX);
        Serial.println(", trying 0x3F...");
        _address = 0x3F; // Update internal address
        _lcd = LiquidCrystal_I2C(_address, 16, 2); // Re-initialize lcd object with new address
        Wire.beginTransmission(_address);
        error = Wire.endTransmission();
    }

    // If LCD is found at either address
    if (error == 0) {
        Serial.print("LCD found at address 0x"); Serial.println(_address, HEX);
        delay(100); // Give LCD time to power up and stabilize
        _lcd.init(); // Initialize the LCD
        _lcd.clear(); // Clear any garbage on display
        _lcd.backlight(); // Turn on backlight
        _initialized = true; // Set initialized flag

        // Create custom characters for WiFi and Sync symbols
        _lcd.createChar(0, _wifiSymbol);
        _lcd.createChar(1, _syncSymbol);

        Serial.println("LCD initialized successfully.");
        return true; // Indicate success
    } else {
        Serial.println("FATAL: LCD not found! Check connections.");
        _initialized = false; // Set initialized flag to false
        return false; // Indicate failure
    }
}

// updateTimeAndDate(): Updates only the time and date portion of the display
// Respects real estate boundaries and doesn't overwrite status icons
void LCDDisplay::updateTimeAndDate(const RTCTime& currentTime) {
    if (!_initialized || _errorDisplayed) return; // Do nothing if LCD is not initialized or error is displayed

    // Get current time components from the RTCTime object
    int currentDay = currentTime.getDayOfMonth();
    int currentHour = currentTime.getHour();
    int currentMinute = currentTime.getMinutes();
    int currentSecond = currentTime.getSeconds();
    int currentMonth = Month2int(currentTime.getMonth()); // Convert Month enum to 1-12 int
    int currentYear = currentTime.getYear();

    // Update date only if day, month, or year has changed to minimize writes
    if (_lastDisplayedDay != currentDay || _lastDisplayedMonth != currentMonth || _lastDisplayedYear != currentYear) {
        _lcd.setCursor(DATE_START, 0); // Set cursor to date start position
        char dateStr[15]; // Buffer for date string: "DD/MMM/YY WWW" (14 chars max)
        // snprintf for safe string formatting
        snprintf(dateStr, sizeof(dateStr), "%02d/%s/%02d %s", 
                currentDay,
                MONTH_NAMES[currentMonth - 1], // MONTH_NAMES is 0-indexed (Jan=0)
                currentYear % 100, // Get last two digits of the year
                DOW_ABBREV[DayOfWeek2int(currentTime.getDayOfWeek(), true) % 7]); // Get abbreviated day of week
        
        // Ensure we don't exceed DATE_END boundary
        String dateDisplay = String(dateStr);
        if (dateDisplay.length() > (DATE_END - DATE_START + 1)) {
            dateDisplay = dateDisplay.substring(0, DATE_END - DATE_START + 1);
        }
        _lcd.print(dateDisplay);
        
        // Update last displayed values
        _lastDisplayedDay = currentDay;
        _lastDisplayedMonth = currentMonth;
        _lastDisplayedYear = currentYear;
    }
    
    // Update time display only if hour, minute, or second has changed
    if (_lastDisplayedHour != currentHour || _lastDisplayedMinute != currentMinute || _lastDisplayedSecond != currentSecond) {
        char timeStr[9]; // Buffer for time string: "HH:MM:SS\0"
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
                currentHour, currentMinute, currentSecond);
        _lcd.setCursor(TIME_START, 1); // Set cursor to time start position
        _lcd.print(timeStr); // Print the formatted time
        
        // Clear the area after time display (positions 8-14) to remove artifacts
        // but preserve the status icon at position 15
        _lcd.setCursor(8, 1);
        _lcd.print("       "); // Clear 7 spaces (positions 8-14)
        
        // Update last displayed values
        _lastDisplayedHour = currentHour;
        _lastDisplayedMinute = currentMinute;
        _lastDisplayedSecond = currentSecond;
    }
}

// updateNetworkStatus(): Updates only the network status icons (WiFi and NTP sync)
// Uses dedicated status real estate positions
void LCDDisplay::updateNetworkStatus(int wifiStatus, unsigned long lastNtpSync, unsigned long ntpSyncInterval) {
    if (!_initialized) return; // Do nothing if LCD is not initialized

    // Update status indicators every 500ms for blinking effect
    if (millis() - _lastBlinkTime >= 500) {
        _lastBlinkTime = millis();
        _statusBlinkState = !_statusBlinkState; // Toggle blink state
        
        // WiFi status icon - top right corner (dedicated position)
        _lcd.setCursor(WIFI_ICON_POS, 0); // Position for WiFi icon
        if (wifiStatus == WL_CONNECTED) {
            _lcd.write(byte(0));  // Display solid WiFi symbol (char 0)
        } else {
            // Blink WiFi symbol if not connected
            _lcd.print(_statusBlinkState ? (char)byte(0) : ' ');  
        }
        
        // NTP sync status icon - bottom right corner (dedicated position)
        _lcd.setCursor(SYNC_ICON_POS, 1); // Position for Sync icon
        // Check if last sync was within the interval (plus a grace period)
        // This makes the sync icon solid for a while after sync, then blinks if overdue.
        if (millis() - lastNtpSync < ntpSyncInterval + (ntpSyncInterval / 4)) { // 25% grace period
            _lcd.write(byte(1));  // Display solid Sync symbol (char 1)
        } else {
            // Blink Sync symbol if sync is overdue
            _lcd.print(_statusBlinkState ? (char)byte(1) : ' ');  
        }
    }
}

// displayError(): Displays an error message on the LCD with overlay capability
void LCDDisplay::displayError(const String& errorMsg, bool overlay, unsigned long duration) {
    if (!_initialized) return;
    
    _errorDisplayed = true;
    _errorStartTime = millis();
    _errorDuration = duration;
    
    if (overlay) {
        // Overlay mode: preserve time/date and show error in available space
        // Use positions 8-14 on line 1 (after time display)
        _lcd.setCursor(8, 1); // Start after time display
        String errorDisplay = errorMsg.substring(0, 8); // Max 8 chars to fit in available space
        _lcd.print(errorDisplay);
    } else {
        // Full error mode: clear display and show error prominently, but preserve status icons
        // Clear lines 0 and 1 except for status icons at position 15
        _lcd.setCursor(0, 0);
        _lcd.print("               "); // Clear positions 0-14 on line 0
        _lcd.setCursor(0, 1);
        _lcd.print("               "); // Clear positions 0-14 on line 1
        
        _lcd.setCursor(ERROR_LINE_START, 0); // Set cursor to top-left
        _lcd.print("ERROR:");
        _lcd.setCursor(ERROR_LINE_START, 1); // Set cursor to bottom-left
        _lcd.print(errorMsg.substring(0, LCD_WIDTH - 1)); // Print error message, but don't overwrite status icon
    }
    _lcd.backlight(); // Ensure backlight is on for error visibility
}

// clearError(): Clears error display and restores normal display, preserving status icons
void LCDDisplay::clearError() {
    if (!_initialized || !_errorDisplayed) return;
    
    _errorDisplayed = false;
    
    // Clear the error area and restore normal display, but preserve status icons
    _lcd.setCursor(0, 0);
    _lcd.print("               "); // Clear positions 0-14 on line 0
    _lcd.setCursor(0, 1);
    _lcd.print("               "); // Clear positions 0-14 on line 1
    
    // Re-initialize display with current time/date
    // Note: This will be updated on next updateTimeAndDate() call
}

// printLine(): Prints a message to a specific line on the LCD, preserving status icons
void LCDDisplay::printLine(uint8_t line, const String& msg) {
    if (!_initialized) return;
    
    // Clear the line except for the status icon at position 15
    _lcd.setCursor(0, line); // Set cursor to the start of the specified line
    _lcd.print("               "); // Clear positions 0-14 (15 spaces)
    // Don't clear position 15 - preserve the status icon
    
    _lcd.setCursor(0, line); // Reset cursor to the start of the line
    _lcd.print(msg.substring(0, LCD_WIDTH - 1)); // Print the new message, but don't overwrite status icon
}

// clear(): Clears the entire LCD display
void LCDDisplay::clear() {
    if (_initialized) {
        _lcd.clear();
        _errorDisplayed = false; // Reset error state when clearing
    }
}

// backlight(): Turns on the LCD backlight
void LCDDisplay::backlight() {
    if (_initialized) _lcd.backlight();
}

// noBacklight(): Turns off the LCD backlight
void LCDDisplay::noBacklight() {
    if (_initialized) _lcd.noBacklight();
} 
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

        // Initialize the smart buffer system
        initializeBuffer();

        Serial.println("LCD initialized successfully.");
        return true; // Indicate success
    } else {
        Serial.println("FATAL: LCD not found! Check connections.");
        _initialized = false; // Set initialized flag to false
        return false; // Indicate failure
    }
}

// updateTimeAndDate(): Updates both time and date portions of the display
// Always updates both lines to ensure status messages get replaced
// Respects real estate boundaries and doesn't overwrite status icons
void LCDDisplay::updateTimeAndDate(const RTCTime& currentTime) {
    if (!_initialized) return; // Do nothing if LCD is not initialized

    // Get current time components from the RTCTime object
    int currentDay = currentTime.getDayOfMonth();
    int currentHour = currentTime.getHour();
    int currentMinute = currentTime.getMinutes();
    int currentSecond = currentTime.getSeconds();
    int currentMonth = Month2int(currentTime.getMonth()); // Convert Month enum to 1-12 int
    int currentYear = currentTime.getYear();

    // Update date display using smart buffer
    char dateStr[15]; // Buffer for date string: "DD/MMM/YY WWW" (14 chars max)
    
    // Calculate day of week directly from UTC Unix timestamp to avoid conversion issues
    // We need to use UTC time for day calculation, not local time
    time_t utcUnixTime = getCurrentUTC(); // Get UTC time directly
    int dayOfWeekInt = ((utcUnixTime / 86400) + 4) % 7; // Unix epoch (1970-01-01) was a Thursday (4)
    
    // snprintf for safe string formatting
    snprintf(dateStr, sizeof(dateStr), "%02d/%s/%02d %s", 
            currentDay,
            MONTH_NAMES[currentMonth - 1], // MONTH_NAMES is 0-indexed (Jan=0)
            currentYear % 100, // Get last two digits of the year
            DOW_ABBREV[dayOfWeekInt]); // Get abbreviated day of week
    
    // Ensure we don't exceed DATE_END boundary
    String dateDisplay = String(dateStr);
    if (dateDisplay.length() > (DATE_END - DATE_START + 1)) {
        dateDisplay = dateDisplay.substring(0, DATE_END - DATE_START + 1);
    }
    
    // Pad date string to fill positions 0-14 on line 0
    while (dateDisplay.length() < 15) {
        dateDisplay += " "; // Pad with spaces
    }
    
    // Create time content for line 1
    char timeStr[9]; // Buffer for time string: "HH:MM:SS\0"
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
            currentHour, currentMinute, currentSecond);
    
    // Pad time string to fill positions 0-14 on line 1
    String timeDisplay = String(timeStr);
    while (timeDisplay.length() < 15) {
        timeDisplay += " "; // Pad with spaces
    }
    
    // Update each line separately (positions 0-14)
    updateBufferArea(0, 0, 0, 14, dateDisplay);  // Line 0: Date
    updateBufferArea(1, 1, 0, 14, timeDisplay);  // Line 1: Time
    
    // Update last displayed values
    _lastDisplayedDay = currentDay;
    _lastDisplayedMonth = currentMonth;
    _lastDisplayedYear = currentYear;
    _lastDisplayedHour = currentHour;
    _lastDisplayedMinute = currentMinute;
    _lastDisplayedSecond = currentSecond;
    
    // Sync any changed regions to the LCD
    syncDirtyRegions();
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
        char wifiChar = (wifiStatus == WL_CONNECTED) ? (char)byte(0) : (_statusBlinkState ? (char)byte(0) : ' ');
        
        // NTP sync status icon - bottom right corner (dedicated position)
        // Check if last sync was within the interval (plus a grace period)
        // This makes the sync icon solid for a while after sync, then blinks if overdue.
        char syncChar = (millis() - lastNtpSync < ntpSyncInterval + (ntpSyncInterval / 4)) ? 
                       (char)byte(1) : (_statusBlinkState ? (char)byte(1) : ' ');
        
        // Update status area (both lines, position 15) - only writes to LCD if changed
        updateStatusArea(0, wifiChar, syncChar);
        
        // Sync any changed regions to the LCD
        syncDirtyRegions();
    }
}





// printLine(): Prints a message to a specific line on the LCD, preserving status icons
void LCDDisplay::printLine(uint8_t line, const String& msg) {
    if (!_initialized) return;
    
    // Pad message to fill positions 0-14 (preserving status icon at position 15)
    String paddedMsg = msg.substring(0, LCD_WIDTH - 1);
    while (paddedMsg.length() < 15) {
        paddedMsg += " "; // Pad with spaces
    }
    
    // Update constrained area (specific line, positions 0-14) - only writes to LCD if changed
    updateBufferArea(line, line, 0, 14, paddedMsg);
    
    // Sync any changed regions to the LCD
    syncDirtyRegions();
}

// Smart Buffer Implementation Methods

// initializeBuffer(): Initialize buffer with current LCD content
void LCDDisplay::initializeBuffer() {
    if (!_initialized) return;
    
    // Initialize buffer with spaces (assume LCD is clear)
    for (int line = 0; line < LCD_HEIGHT; line++) {
        for (int col = 0; col < LCD_WIDTH; col++) {
            _buffer[line][col] = ' ';
            _charDirty[line][col] = false;
        }
        _buffer[line][LCD_WIDTH] = '\0'; // Null terminate
        _lineDirty[line] = false;
    }
    _bufferInitialized = true;
}

// updateBufferArea(): Update constrained area in buffer and mark as dirty
void LCDDisplay::updateBufferArea(uint8_t startLine, uint8_t endLine, uint8_t startCol, uint8_t endCol, const String& content) {
    if (!_bufferInitialized || startLine >= LCD_HEIGHT || endLine >= LCD_HEIGHT || 
        startCol >= LCD_WIDTH || endCol >= LCD_WIDTH) return;
    
    uint8_t contentIndex = 0;
    
    // Update each line in the area
    for (uint8_t line = startLine; line <= endLine && line < LCD_HEIGHT; line++) {
        for (uint8_t col = startCol; col <= endCol && col < LCD_WIDTH; col++) {
            char newChar = (contentIndex < content.length()) ? content[contentIndex++] : ' ';
            
            if (_buffer[line][col] != newChar) {
                _buffer[line][col] = newChar;
                _charDirty[line][col] = true;
                _lineDirty[line] = true;
            }
        }
    }
}

// updateStatusArea(): Update status icons (position 15) on both lines
void LCDDisplay::updateStatusArea(uint8_t line, char wifiChar, char syncChar) {
    if (!_bufferInitialized || line >= LCD_HEIGHT) return;
    
    // Update WiFi icon on line 0, position 15
    if (_buffer[0][WIFI_ICON_POS] != wifiChar) {
        _buffer[0][WIFI_ICON_POS] = wifiChar;
        _charDirty[0][WIFI_ICON_POS] = true;
        _lineDirty[0] = true;
    }
    
    // Update Sync icon on line 1, position 15
    if (_buffer[1][SYNC_ICON_POS] != syncChar) {
        _buffer[1][SYNC_ICON_POS] = syncChar;
        _charDirty[1][SYNC_ICON_POS] = true;
        _lineDirty[1] = true;
    }
}

// clearBufferLine(): Clear a line in buffer and mark as dirty
void LCDDisplay::clearBufferLine(uint8_t line) {
    if (!_bufferInitialized || line >= LCD_HEIGHT) return;
    
    for (int col = 0; col < LCD_WIDTH; col++) {
        if (_buffer[line][col] != ' ') {
            _buffer[line][col] = ' ';
            _charDirty[line][col] = true;
            _lineDirty[line] = true;
        }
    }
}

// clearBuffer(): Clear entire buffer and mark all lines as dirty
void LCDDisplay::clearBuffer() {
    if (!_initialized) return;
    
    for (int line = 0; line < LCD_HEIGHT; line++) {
        clearBufferLine(line);
    }
}

// syncDirtyRegions(): Sync only changed regions to LCD
void LCDDisplay::syncDirtyRegions() {
    if (!_initialized || !_bufferInitialized) return;
    
    for (int line = 0; line < LCD_HEIGHT; line++) {
        if (_lineDirty[line]) {
            // Write entire line if any char changed
            _lcd.setCursor(0, line);
            _lcd.print(_buffer[line]);
            _lineDirty[line] = false;
            
            // Clear dirty flags for this line
            for (int col = 0; col < LCD_WIDTH; col++) {
                _charDirty[line][col] = false;
            }
        }
    }
}

// debugPrintBuffer(): Debug helper to print buffer contents
void LCDDisplay::debugPrintBuffer() {
    Serial.println("=== LCD Buffer Contents ===");
    for (int line = 0; line < LCD_HEIGHT; line++) {
        Serial.print("Line "); Serial.print(line); Serial.print(": '");
        Serial.print(_buffer[line]);
        Serial.print("' Dirty: "); Serial.println(_lineDirty[line]);
    }
    Serial.println("==========================");
}

// clear(): Clears the entire LCD display
void LCDDisplay::clear() {
    if (_initialized) {
        _lcd.clear();
        clearBuffer(); // Clear buffer to match LCD
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
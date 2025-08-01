#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Wire.h> // For I2C communication
#include <LiquidCrystal_I2C.h> // For LCD control
#include <RTC.h> // For RTCTime object (passed for display)
#include <WiFiS3.h> // For WL_CONNECTED status (used in updateNetworkStatus)

// Global utility functions that LCDDisplay might use for formatting
// Month2int and DayOfWeek2int are provided by the RTC library
// These arrays are defined in TimeUtils.cpp
extern const char* const MONTH_NAMES[];
extern const char* const DOW_ABBREV[];

// LCD Display Layout Constants
// 16x2 LCD: 16 characters wide, 2 lines high
const uint8_t LCD_WIDTH = 16;
const uint8_t LCD_HEIGHT = 2;

// Real Estate Boundaries
const uint8_t DATE_START = 0;      // Date starts at position 0
const uint8_t DATE_END = 13;       // Date ends at position 13 (14 chars total)
const uint8_t TIME_START = 0;      // Time starts at position 0
const uint8_t TIME_END = 7;        // Time ends at position 7 (8 chars total)
const uint8_t STATUS_START = 14;   // Status icons start at position 14
const uint8_t STATUS_END = 15;     // Status icons end at position 15

// Status Icon Positions
const uint8_t WIFI_ICON_POS = 15;  // WiFi icon at position 15, line 0
const uint8_t SYNC_ICON_POS = 15;  // Sync icon at position 15, line 1

// Error Display Constants
const uint8_t ERROR_LINE_START = 0;
const uint8_t ERROR_LINE_END = 15;

class LCDDisplay {
private:
    LiquidCrystal_I2C _lcd;
    bool _initialized;
    uint8_t _address;

    // Custom characters (as members to be created once)
    byte _wifiSymbol[8] = {
        B00000,
        B00000,
        B00001,
        B00101,
        B10101,
        B00000,
        B00000,
        B00000
    };

    byte _syncSymbol[8] = {
        B00000,
        B00000,
        B01110,
        B10001,
        B10101,
        B10001,
        B01110,
        B00000
    };

    // Keep track of last displayed values for optimization (for time display)
    int _lastDisplayedSecond = -1;
    int _lastDisplayedMinute = -1;
    int _lastDisplayedHour = -1;
    int _lastDisplayedDay = -1;
    int _lastDisplayedMonth = -1;
    int _lastDisplayedYear = -1;

    // For status icon blinking
    bool _statusBlinkState = false;
    unsigned long _lastBlinkTime = 0;
    
    // Error display state
    bool _errorDisplayed = false;
    unsigned long _errorStartTime = 0;
    unsigned long _errorDuration = 3000; // 3 seconds default
    
public:
    // Constructor: Takes the I2C address of the LCD
    LCDDisplay(uint8_t address = 0x27);

    // Initializes the LCD hardware. Returns true on success, false on failure.
    bool begin(); 
    
    // Updates only the time and date portion of the display.
    // Respects real estate boundaries and doesn't overwrite status icons.
    void updateTimeAndDate(const RTCTime& currentTime);

    // Updates only the network status icons (WiFi and NTP sync).
    // Uses dedicated status real estate positions.
    void updateNetworkStatus(int wifiStatus, unsigned long lastNtpSync, unsigned long ntpSyncInterval);

    // Displays an error message on the LCD with overlay capability.
    // If overlay is true, preserves time/date and only shows error in available space.
    void displayError(const String& errorMsg, bool overlay = false, unsigned long duration = 3000);

    // Prints a message to a specific line on the LCD, clearing the line first.
    void printLine(uint8_t line, const String& msg);

    // Clears the entire LCD display.
    void clear();

    // Turns on the LCD backlight.
    void backlight();

    // Turns off the LCD backlight.
    void noBacklight();

    // Utility method to check if error is currently being displayed
    bool isErrorDisplayed() const { return _errorDisplayed; }

    // Utility method to clear error display and restore normal display
    void clearError();
};

#endif // LCD_DISPLAY_H 
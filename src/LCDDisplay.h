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
    
public:
    // Constructor: Takes the I2C address of the LCD
    LCDDisplay(uint8_t address = 0x27);

    // Initializes the LCD hardware. Returns true on success, false on failure.
    bool begin(); 
    
    // Updates only the time and date portion of the display.
    void updateTimeAndDate(const RTCTime& currentTime);

    // Updates only the network status icons (WiFi and NTP sync).
    void updateNetworkStatus(int wifiStatus, unsigned long lastNtpSync, unsigned long ntpSyncInterval);

    // Displays an error message on the LCD.
    void displayError(const String& errorMsg);

    // Prints a message to a specific line on the LCD, clearing the line first.
    void printLine(uint8_t line, const String& msg);

    // Clears the entire LCD display.
    void clear();

    // Turns on the LCD backlight.
    void backlight();

    // Turns off the LCD backlight.
    void noBacklight();
};

#endif // LCD_DISPLAY_H 
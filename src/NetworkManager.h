#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFiS3.h>   // For WiFi functions (client, AP, status)
// #include <WebServer.h> // Not used for R4, manual HTTP handling with WiFiServer
#include <RTC.h>      // For updating the onboard RTC
#include <EEPROM.h>   // For storing/retrieving credentials
#include <time.h>     // For time_t, struct tm, etc.
#include "TimeUtils.h" // For calculateDST and time utility functions

// EEPROM addresses for WiFi credentials (must be consistent with main.cpp if used there)
const int EEPROM_ADDR_WIFI_CRED_START = 100; // Starting address for WiFi credentials struct
const int EEPROM_ADDR_TIME_ZONE_OFFSET = 200; // Address for time zone offset in hours
const int EEPROM_ADDR_USE_DST_FLAG = 204; // Address for boolean flag (use one byte)

// Structure for storing WiFi credentials in EEPROM
struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool isValid; // Flag to indicate if the stored credentials are valid
};

class NetworkManager {
private:
    WiFiCredentials _credentials; // Stores current WiFi credentials (from EEPROM or AP)
    bool _configModeRequired;     // Flag: true if AP mode is needed on startup
    
    // Web Server for Captive Portal
    WiFiServer _server; // WiFiServer for manual HTTP handling
    const char* _apSsid; // SSID for the Access Point (e.g., "ClockSetup")
    
    // NTP related
    WiFiUDP _udpClient; // UDP client for NTP
    IPAddress _ntpServerIP; // NTP server IP address
    const unsigned int _localPort; // Local UDP port
    byte _ntpPacketBuffer[48]; // Buffer for NTP packet

    // Timezone and DST settings
    int _timeZoneOffsetHours; // Standard (non-DST) offset from UTC in hours (e.g., -5 for EST)
    bool _useDST;             // Flag to enable/disable automatic DST calculation

    // Retry strategies
    const unsigned long _wifiConnectTimeout; // Max time to wait for WiFi connection
    const int _maxNtpRetries;
    const unsigned long _ntpRetryDelay;
    const int _wifiReconnectRetries; // Not explicitly used yet, but good for future
    const unsigned long _wifiReconnectDelay; // Not explicitly used yet, but good for future
    const unsigned long _ntpSyncInterval; // How often to resync NTP

    unsigned long _lastNTPSyncTime; // millis() timestamp of last successful NTP sync

    // Private helper methods for captive portal (manual HTTP handling)
    void _handleRootRequest(WiFiClient client);
    void _handleSaveRequest(WiFiClient client, String requestLine);
    void _sendHttpResponse(WiFiClient client, int statusCode, const char* contentType, const String& content);
    String _urlDecode(String str);
    bool _testWiFiConnection(const char* testSsid, const char* testPass);

public:
    // Constructor
    NetworkManager(
        const char* apSsid, // SSID for the captive portal AP
        IPAddress ntpServerIP = IPAddress(129, 6, 15, 28), // Default to time.nist.gov
        unsigned int localPort = 2390,
        unsigned long wifiConnectTimeout = 30000,
        int maxNtpRetries = 3,
        unsigned long ntpRetryDelay = 5000,
        int wifiReconnectRetries = 3,
        unsigned long wifiReconnectDelay = 10000,
        unsigned long ntpSyncInterval = 3600000UL, // 1 hour
        int timeZoneOffsetHours = -5, // Default to EST (-5)
        bool useDST = true             // Default to using DST
    );

    // Initial setup (load credentials and timezone)
    void begin();

    // Returns true if captive portal setup is required
    bool needsConfiguration() const;

    // Sets up the Access Point for configuration mode
    void setupAccessPoint();

    // Handles client connections and requests in captive portal mode.
    // Returns true if new credentials are successfully saved and tested,
    // and the system should transition out of config mode.
    // Pass by reference 'errorMessage' to provide feedback to main loop/LCD.
    bool handleConfigPortal(String& errorMessage);

    // Attempts to connect to WiFi using stored credentials. Returns true on success.
    bool ensureConnection();

    // Attempts to synchronize RTC with NTP server, with retries. Returns true on success.
    bool syncTimeWithRTC(RTClock& rtcInstance);

    // Checks if it's time for a periodic NTP sync and performs it if needed.
    void periodicNtpSync(RTClock& rtcInstance);

    // Getters for status info (for LCD display)
    int getWiFiStatus() const;
    unsigned long getLastNtpSyncTime() const; // Renamed for clarity
    unsigned long getNtpSyncInterval() const;
    const char* getSSID() const; // For displaying connected SSID

    // Clear saved WiFi credentials from EEPROM
    void clearWiFiCredentials();

    // Stops the Access Point (if active)
    void stopAccessPoint();

    // Additional methods needed by StateManager
    void startConfigurationMode();
    void stopConfigurationMode();
    bool isConfigurationComplete() const;
    bool connectToWiFi();
    bool syncTimeWithNTP();
    bool isWiFiConnected() const;
    bool isNTPSyncNeeded() const;
    void resetNtpSyncCounter(); // Reset NTP sync counter to defer sync for another interval
    void saveCredentials(const char* newSsid, const char* newPassword);
    
    // Getters for timezone settings
    int getTimeZoneOffset() const { return _timeZoneOffsetHours; }
    bool getUseDST() const { return _useDST; }
};

#endif // NETWORK_MANAGER_H 
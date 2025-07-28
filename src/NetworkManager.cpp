#include "NetworkManager.h"
#include <Arduino.h> // For millis(), Serial.print, delay, etc.
#include <EEPROM.h>    // For EEPROM.get/put
#include <RTC.h>       // For RTC.setTime()
#include <string.h>    // For strncpy, memset
#include "TimeUtils.h" // For calculateDST





// --- Constructor ---
NetworkManager::NetworkManager(const char* apSsid, IPAddress ntpServerIP, unsigned int localPort,
                               unsigned long wifiConnectTimeout, int maxNtpRetries,
                               unsigned long ntpRetryDelay, int wifiReconnectRetries,
                               unsigned long wifiReconnectDelay, unsigned long ntpSyncInterval,
                               int timeZoneOffsetHours, bool useDST)
    : _server(80), // Initialize WiFiServer on port 80
      _apSsid(apSsid), // Store AP SSID
      _ntpServerIP(ntpServerIP),
      _localPort(localPort),
      _timeZoneOffsetHours(timeZoneOffsetHours), // Store timezone offset
      _useDST(useDST),                         // Store DST flag
      _wifiConnectTimeout(wifiConnectTimeout),
      _maxNtpRetries(maxNtpRetries),
      _ntpRetryDelay(ntpRetryDelay),
      _wifiReconnectRetries(wifiReconnectRetries),
      _wifiReconnectDelay(wifiReconnectDelay),
      _ntpSyncInterval(ntpSyncInterval),
      _lastNTPSyncTime(0),
      _configModeRequired(false) // Default to false, determined in begin()
{
    // Initialize credentials buffer to nulls
    memset(_credentials.ssid, 0, sizeof(_credentials.ssid));
    memset(_credentials.password, 0, sizeof(_credentials.password));
    _credentials.isValid = false;
}

// --- Begin Method ---
void NetworkManager::begin() {
    Serial.println("NetworkManager::begin() called.");
    
    // Load WiFi credentials from EEPROM
    EEPROM.get(EEPROM_ADDR_WIFI_CRED_START, _credentials);
    
    // Load timezone offset and DST flag from EEPROM
    EEPROM.get(EEPROM_ADDR_TIME_ZONE_OFFSET, _timeZoneOffsetHours);
    EEPROM.get(EEPROM_ADDR_USE_DST_FLAG, _useDST);

    // Basic validation of loaded credentials
    // Check isValid flag and non-zero length for SSID
    if (_credentials.isValid && strlen(_credentials.ssid) > 0 && strlen(_credentials.ssid) < sizeof(_credentials.ssid)) {
        // Additional check for corrupted data (non-printable characters)
        bool hasCorruptedData = false;
        for (int i = 0; i < strlen(_credentials.ssid); i++) {
            if (_credentials.ssid[i] < 32 || _credentials.ssid[i] > 126) {
                hasCorruptedData = true;
                break;
            }
        }
        
        if (hasCorruptedData) {
            Serial.println("✗ WiFi credentials corrupted in EEPROM. Clearing and entering config mode.");
            _configModeRequired = true;
            // Clear corrupted data
            memset(_credentials.ssid, 0, sizeof(_credentials.ssid));
            memset(_credentials.password, 0, sizeof(_credentials.password));
            _credentials.isValid = false;
            // Save cleared credentials to EEPROM
            EEPROM.put(EEPROM_ADDR_WIFI_CRED_START, _credentials);
        } else {
            Serial.println("✓ Valid WiFi credentials found in EEPROM.");
            Serial.print("Loaded SSID: "); Serial.println(_credentials.ssid);
            _configModeRequired = false; // No config mode needed initially
        }
    } else {
        Serial.println("✗ No valid WiFi credentials found in EEPROM. Entering config mode.");
        _configModeRequired = true; // Force config mode
        // Clear invalid data just in case
        memset(_credentials.ssid, 0, sizeof(_credentials.ssid));
        memset(_credentials.password, 0, sizeof(_credentials.password));
        _credentials.isValid = false;
    }
    
    Serial.print("Loaded Time Zone Offset: "); Serial.print(_timeZoneOffsetHours); Serial.println(" hours");
    Serial.print("Loaded Use DST: "); Serial.println(_useDST ? "Yes" : "No");
}

// --- Getter for Config Mode Status ---
bool NetworkManager::needsConfiguration() const {
    return _configModeRequired;
}

// --- Setup Access Point ---
void NetworkManager::setupAccessPoint() {
    Serial.println("\n--- Setting up Access Point ---");
    
    // Stop any existing connections and WiFi
    WiFi.end(); // Disconnects from any STA and stops SoftAP
    delay(500);
    
    // Check WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        _configModeRequired = true; // Stay in config mode or error
        return;
    }
    
    // Print firmware version (optional, good for debug)
    String fv = WiFi.firmwareVersion();
    Serial.print("Firmware version: ");
    Serial.println(fv);
    
    // Configure AP with static IP (captive portals often use a fixed IP)
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.config(local_ip, gateway, subnet);
    
    // Create AP
    Serial.print("Creating access point named: ");
    Serial.println(_apSsid);
    
    // WiFi.beginAP returns wl_status_t
    if (WiFi.beginAP(_apSsid) != WL_AP_LISTENING) {
        Serial.println("Creating access point failed!");
        // Consider setting _configModeRequired = true; or entering error state
        return;
    }
    
    // Wait for AP to start listening
    unsigned long apStartTime = millis();
    while (WiFi.status() != WL_AP_LISTENING && (millis() - apStartTime < 10000)) { // 10 second timeout
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_AP_LISTENING) {
        Serial.println("\nAP is now listening!");
        Serial.println("\n--- AP Setup Complete ---");
        Serial.println("------------------------");
        Serial.print("Network Name: "); Serial.println(_apSsid);
        Serial.print("IP Address: "); Serial.println(WiFi.localIP());
        Serial.println("------------------------");
        Serial.println("To configure WiFi:");
        Serial.println("1. Connect to '" + String(_apSsid) + "' network");
        Serial.println("2. Visit http://192.168.4.1");
        Serial.println("------------------------");
        
        // Start web server
        _server.begin();
        _udpClient.stop(); // Ensure UDP client is stopped if AP is active
        Serial.println("Web server started for captive portal.");
    } else {
        Serial.println("\nAP failed to start listening within timeout!");
        // Force config mode or error state if AP fails
        _configModeRequired = true; 
        

    }
}

// --- Stop Access Point ---
void NetworkManager::stopAccessPoint() {
    // For Arduino R4 WiFi, just call WiFi.end() to stop both STA and AP modes
    if (WiFi.status() == WL_AP_LISTENING) { // Check if AP is active
        Serial.println("Stopping AP.");
        WiFi.end(); // Stops both STA and AP modes
        delay(500);
        
        // Reset WiFi configuration to allow DHCP in client mode
        // For Arduino R4 WiFi, we need to completely reset the WiFi module
        WiFi.disconnect();
        delay(1000); // Give more time for complete reset
    }
}

// --- Connect to WiFi (Client Mode) ---
bool NetworkManager::ensureConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        return true; // Already connected
    }
    
    Serial.println("\n--- Attempting WiFi Client Connection ---");
    Serial.print("Target SSID: "); Serial.println(_credentials.ssid);

    // Stop any existing connections and WiFi (like the working version)
    Serial.println("Stopping any existing WiFi connections...");
    WiFi.end();
    delay(1000);
    
    // Try to connect (like the working version)
    Serial.println("Attempting to connect...");
    WiFi.begin(_credentials.ssid, _credentials.password);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < _wifiConnectTimeout) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi Connected!");
        
        // Wait for DHCP to assign an IP address
        Serial.println("Waiting for DHCP to assign IP address...");
        unsigned long dhcpStartTime = millis();
        IPAddress currentIP = WiFi.localIP();
        
        while (currentIP[0] == 0 && (millis() - dhcpStartTime < 15000)) { // Wait up to 15 seconds for DHCP
            delay(1000);
            currentIP = WiFi.localIP();
            Serial.print("Waiting for DHCP... Current IP: ");
            Serial.println(currentIP);
        }
        
        Serial.print("Final IP Address: "); Serial.println(currentIP);
        
        if (currentIP[0] == 0) {
            Serial.println("Warning: DHCP failed to assign IP address, but continuing...");
        }
        
        return true;
    } else {
        Serial.println("\n✗ WiFi Connection Failed!");
        Serial.print("Final Status: "); Serial.println(WiFi.status());
        _configModeRequired = true; // Force back to config mode if connection fails
        return false;
    }
}

// --- Synchronize RTC with NTP ---
bool NetworkManager::syncTimeWithRTC(RTClock& rtcInstance) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("NTP Sync failed: WiFi not connected.");
        return false;
    }

    Serial.println("\n--- Attempting NTP Time Sync ---");
    
    // Initialize UDP client for NTP
    _udpClient.stop(); // Stop any previous UDP sessions
    if (!_udpClient.begin(_localPort)) {
        Serial.println("✗ Failed to start UDP client for NTP.");
        return false;
    }
    
    for (int i = 0; i < _maxNtpRetries; i++) {
        Serial.print("NTP attempt "); Serial.print(i + 1);
        Serial.print(" of "); Serial.print(_maxNtpRetries);
        
        // Clear the packet buffer
        memset(_ntpPacketBuffer, 0, sizeof(_ntpPacketBuffer));
        
        // Set NTP request headers (LI, Version, Mode)
        _ntpPacketBuffer[0] = 0b11100011;   
        
        // Send the packet
        Serial.println(" - Sending NTP request...");
        _udpClient.beginPacket(_ntpServerIP, 123); // NTP port is 123
        _udpClient.write(_ntpPacketBuffer, sizeof(_ntpPacketBuffer));
        _udpClient.endPacket();
        
        unsigned long startWait = millis();
        while (millis() - startWait < 2000) { // Wait up to 2 seconds for response
            if (_udpClient.parsePacket()) {
                _udpClient.read(_ntpPacketBuffer, sizeof(_ntpPacketBuffer));
                Serial.println("✓ Received NTP response.");
                
                // Extract NTP time (seconds since Jan 1 1900) to Unix time
                unsigned long highWord = word(_ntpPacketBuffer[40], _ntpPacketBuffer[41]);
                unsigned long lowWord = word(_ntpPacketBuffer[42], _ntpPacketBuffer[43]);
                unsigned long secsSince1900 = highWord << 16 | lowWord;
                const unsigned long seventyYears = 2208988800UL; // Seconds from 1900 to 1970
                unsigned long epoch = secsSince1900 - seventyYears; // Convert to Unix epoch time (UTC)
                
                // --- Apply Time Zone Offset and DST ---
                bool isDST_now = false;
                if (_useDST) {
                    // Calculate DST based on the *UTC* time plus the *standard* offset.
                    // This is because DST rules are often defined against local time,
                    // so we need to know what local date/time this UTC epoch would be.
                    RTCTime utcPlusStandardOffsetTime(epoch + (long)_timeZoneOffsetHours * 3600L);
                    isDST_now = calculateDST(utcPlusStandardOffsetTime, _timeZoneOffsetHours); // Use TimeUtils calculateDST
                }
                
                // Store UTC time in RTC (timezone/DST conversion happens only for display)
                RTCTime timeToSet(epoch); // Create RTCTime object with UTC epoch
                rtcInstance.setTime(timeToSet); // Set the onboard RTC to UTC
                _lastNTPSyncTime = millis();    // Record sync time (millis() timestamp)
                
                Serial.println("✓ RTC synchronized with network time!");
                Serial.print("Current UTC Unix Time (received): "); Serial.println(secsSince1900 - seventyYears);
                Serial.print("Applied TZ Offset: "); Serial.print(_timeZoneOffsetHours); Serial.println(" hours");
                Serial.print("DST Active: "); Serial.println(isDST_now ? "Yes" : "No");
                Serial.print("Set RTC to (UTC): "); Serial.println(timeToSet.toString());
                return true;
            }
            delay(10);
        }
        Serial.println("✗ NTP attempt failed, retrying...");
        delay(_ntpRetryDelay);
    }
    
    Serial.println("✗ All NTP attempts failed!");
    return false;
}

// --- Periodic NTP Sync Check ---
void NetworkManager::periodicNtpSync(RTClock& rtcInstance) {
    if (WiFi.status() == WL_CONNECTED && (millis() - _lastNTPSyncTime >= _ntpSyncInterval)) {
        Serial.println("\n--- Periodic NTP Sync Triggered ---");
        syncTimeWithRTC(rtcInstance); // Perform sync
    }
}

// --- Getters for Status ---
int NetworkManager::getWiFiStatus() const {
    return WiFi.status();
}

unsigned long NetworkManager::getLastNtpSyncTime() const {
    return _lastNTPSyncTime;
}

unsigned long NetworkManager::getNtpSyncInterval() const {
    return _ntpSyncInterval;
}

const char* NetworkManager::getSSID() const {
    return _credentials.ssid;
}

// --- Clear WiFi Credentials ---
void NetworkManager::clearWiFiCredentials() {
    Serial.println("NetworkManager::clearWiFiCredentials() called.");
    _credentials.isValid = false;
    memset(_credentials.ssid, 0, sizeof(_credentials.ssid));
    memset(_credentials.password, 0, sizeof(_credentials.password));
    EEPROM.put(EEPROM_ADDR_WIFI_CRED_START, _credentials);
    // Also clear timezone info
    _timeZoneOffsetHours = -5; // Reset to default
    _useDST = true; // Reset to default
    EEPROM.put(EEPROM_ADDR_TIME_ZONE_OFFSET, _timeZoneOffsetHours);
    EEPROM.put(EEPROM_ADDR_USE_DST_FLAG, _useDST);
    Serial.println("✓ WiFi credentials and timezone settings cleared from EEPROM.");
}

// --- Captive Portal Implementation Helper Methods ---

void NetworkManager::_sendHttpResponse(WiFiClient client, int statusCode, const char* contentType, const String& content) {
    client.println("HTTP/1.1 " + String(statusCode) + " OK"); 
    client.println("Content-type:" + String(contentType));
    client.println("Connection: close"); // Tell client connection will be closed
    client.println("Content-Length: " + String(content.length())); // Content-Length header
    client.println(); // Blank line to end headers
    client.print(content); // Actual content
    delay(1); // Give the client time to receive the data
    client.stop(); // Close the connection
}

// Captive Portal HTML forms
// Note: Time zone input field added.
const char* CAPTIVE_PORTAL_HTML_FORM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Clock WiFi Setup</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: Arial; margin: 20px; text-align: center;}
    h1 { color: #333; }
    form { background: #f4f4f4; padding: 20px; border-radius: 8px; display: inline-block; }
    div { margin: 10px 0; }
    input[type="text"], input[type="password"] { 
        width: 180px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; 
    }
    input[type="submit"] { 
        background-color: #4CAF50; color: white; padding: 10px 15px; border: none; 
        border-radius: 4px; cursor: pointer; font-size: 16px; 
    }
    input[type="submit"]:hover { background-color: #45a049; }
    .status { color: red; }
  </style>
</head>
<body>
  <h1>Clock WiFi Setup</h1>
  <p>Connect to this hotspot and enter your home WiFi details below.</p>
  <form method='get' action='/'>
    <div>SSID:<br><input type='text' name='ssid' required></div>
    <div>Password:<br><input type='password' name='pass'></div>
    <div>Time Zone Offset (hours from UTC, e.g., -5 for EST):<br><input type='number' name='tz' value='-5' required></div>
    <div>Use DST:<br><input type='checkbox' name='usedst' checked></div>
    <div><input type='submit' value='Connect'></div>
  </form>
  <p class="status"></p>
</body>
</html>
)=====";

const char* CAPTIVE_PORTAL_HTML_TESTING = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Testing WiFi</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: Arial; margin: 20px; text-align: center;}
    h1 { color: #333; }
  </style>
</head>
<body>
  <h1>Testing Connection...</h1>
  <p>Attempting to connect to your WiFi network.</p>
  <p>Please wait...</p>
</body>
</html>
)=====";

const char* CAPTIVE_PORTAL_HTML_SUCCESS = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Success!</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: Arial; margin: 20px; text-align: center;}
    h1 { color: #333; }
    .success { color: green; }
  </style>
</head>
<body>
  <h1><span class="success">Success!</span></h1>
  <p>WiFi settings saved and connection established.</p>
  <p>Clock will now synchronize time and begin operation.</p>
</body>
</html>
)=====";

const char* CAPTIVE_PORTAL_HTML_FAILED = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Connection Failed</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body { font-family: Arial; margin: 20px; text-align: center;}
    h1 { color: #333; }
    .error { color: red; }
  </style>
</head>
<body>
  <h1><span class="error">Connection Failed</span></h1>
  <p>Could not connect to the network with the provided details.</p>
  <p>Please check your SSID and password and try again.</p>
  <form method='get' action='/'>
    <div>SSID:<br><input type='text' name='ssid' required></div>
    <div>Password:<br><input type='password' name='pass'></div>
    <div>Time Zone Offset (hours from UTC, e.g., -5 for EST):<br><input type='number' name='tz' value='-5' required></div>
    <div>Use DST:<br><input type='checkbox' name='usedst' checked></div>
    <div><input type='submit' value='Connect'></div>
  </form>
</body>
</html>
)=====";


// --- URL Decode Utility ---
String NetworkManager::_urlDecode(String str) {
    String ret = "";
    char ch;
    int i, ii, len = str.length();
    
    for (i = 0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        } else {
            // Convert hex to char
            char hexByte[3];
            hexByte[0] = str[i+1];
            hexByte[1] = str[i+2];
            hexByte[2] = '\0';
            unsigned int value;
            sscanf(hexByte, "%x", &value);
            ch = static_cast<char>(value);
            ret += ch;
            i += 2;
        }
    }
    return ret;
}

// --- Test WiFi Connection (during captive portal setup) ---
bool NetworkManager::_testWiFiConnection(const char* testSsid, const char* testPass) {
    Serial.println("\n--- Testing WiFi Connection ---");
    Serial.print("Attempting to connect to SSID: "); Serial.println(testSsid);
    
    // Stop AP mode and try to connect (like the working version)
    Serial.println("Stopping AP mode...");
    WiFi.end();
    delay(1000);
    
    Serial.println("Attempting to connect...");
    WiFi.begin(testSsid, testPass);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < _wifiConnectTimeout) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ Test Connection Successful!");
        WiFi.end(); // Disconnect after test to allow AP mode to restart
        return true;
    } else {
        Serial.println("\n✗ Test Connection Failed!");
        return false;
    }
}

// --- Save WiFi Credentials and Timezone to EEPROM ---
void NetworkManager::saveCredentials(const char* newSsid, const char* newPassword) {
    Serial.println("NetworkManager::saveCredentials() called.");
    strncpy(_credentials.ssid, newSsid, sizeof(_credentials.ssid) - 1);
    strncpy(_credentials.password, newPassword, sizeof(_credentials.password) - 1);
    _credentials.ssid[sizeof(_credentials.ssid) - 1] = '\0'; // Ensure null termination
    _credentials.password[sizeof(_credentials.password) - 1] = '\0';
    _credentials.isValid = true;
    
    EEPROM.put(EEPROM_ADDR_WIFI_CRED_START, _credentials);
    // Timezone and DST are already updated in NetworkManager::handleConfigPortal
    EEPROM.put(EEPROM_ADDR_TIME_ZONE_OFFSET, _timeZoneOffsetHours);
    EEPROM.put(EEPROM_ADDR_USE_DST_FLAG, _useDST);

    Serial.println("✓ Credentials and Timezone settings saved to EEPROM.");
}

// --- Handle Captive Portal (main loop entry) ---
bool NetworkManager::handleConfigPortal(String& errorMessage) {
    static unsigned long lastDebugPrint = 0;
    static unsigned long lastAPRestart = 0;
    static int restartCount = 0;
    
    if (millis() - lastDebugPrint > 5000) {
        lastDebugPrint = millis();
        Serial.print("AP IP: "); Serial.println(WiFi.localIP());
        Serial.print("AP Status: "); Serial.println(WiFi.status());
    }

    // Only attempt AP restart once every 30 seconds to prevent restart loops
    // Check for both WL_AP_LISTENING (7) and WL_AP_CONNECTED (8) as valid AP states
    if (WiFi.status() != WL_AP_LISTENING && WiFi.status() != WL_AP_CONNECTED && (millis() - lastAPRestart > 30000)) {
        Serial.println("AP not listening, attempting to start AP...");
        lastAPRestart = millis();
        restartCount++;
        
        if (restartCount > 3) {
            Serial.println("Too many AP restarts, giving up!");
            errorMessage = "AP Unstable";
            return false;
        }
        
        setupAccessPoint(); // Call the method to set up AP
        if (WiFi.status() != WL_AP_LISTENING && WiFi.status() != WL_AP_CONNECTED) { // If it still failed to listen
            Serial.println("Failed to re-setup AP!");
            errorMessage = "AP Failed to Start";
            return false; // Indicate failure to transition
        }
    }
    
    WiFiClient client = _server.available();
    if (client) {
        Serial.println("\n--- New Client Connected to AP ---");
        Serial.print("Client IP: "); Serial.println(client.remoteIP());
        String currentLine = "";
        unsigned long connectionStartTime = millis();
        
        while (client.connected() && (millis() - connectionStartTime < 5000)) { // Timeout for reading request
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    // Blank line indicates end of HTTP headers. Process the request line.
                    if (currentLine.length() == 0) {
                        // This indicates a blank line after headers or start of a new request
                        break; // Exit loop, let client.stop() handle.
                    }
                    
                    if (currentLine.startsWith("GET /?ssid=")) { // Form submission for config
                        // Extract time zone and DST from the form data
                        int tzStart = currentLine.indexOf("tz=") + 3;
                        int tzEnd = currentLine.indexOf("&usedst=");
                        String tzStr = (tzStart != -1 && tzEnd != -1) ? currentLine.substring(tzStart, tzEnd) : "";
                        
                        int usedstStart = currentLine.indexOf("usedst=") + 7;
                        int usedstEnd = currentLine.indexOf(" HTTP");
                        String usedstStr = (usedstStart != -1 && usedstEnd != -1) ? currentLine.substring(usedstStart, usedstEnd) : "";

                        _timeZoneOffsetHours = atoi(_urlDecode(tzStr).c_str());
                        _useDST = (usedstStr == "on"); // Check if checkbox was 'on'
                        Serial.print("Received Time Zone Offset: "); Serial.println(_timeZoneOffsetHours);
                        Serial.print("Received Use DST: "); Serial.println(_useDST ? "Yes" : "No");

                        _handleSaveRequest(client, currentLine); // Handle save with new timezone data
                        return _configModeRequired == false; // If configModeRequired is now false, success!
                    } else if (currentLine.startsWith("GET /")) { // Any other GET request (likely captive portal redirect or initial load)
                        _handleRootRequest(client);
                        break; // Done with this client after sending form
                    }
                    currentLine = ""; // Reset for next line
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        Serial.println("Client request timeout or no request received.");
        client.stop(); // Ensure client is stopped
    }
    return false; // Not yet successfully configured
}

// --- Additional methods needed by StateManager ---

void NetworkManager::startConfigurationMode() {
    setupAccessPoint();
}

void NetworkManager::stopConfigurationMode() {
    stopAccessPoint();
}

bool NetworkManager::isConfigurationComplete() const {
    return !_configModeRequired;
}

bool NetworkManager::connectToWiFi() {
    return ensureConnection();
}

bool NetworkManager::syncTimeWithNTP() {
    // This method needs an RTC reference, but StateManager doesn't have access to it
    // We'll need to modify the StateManager to pass the RTC reference
    // For now, return false to indicate sync is needed but not performed
    return false; // Placeholder - needs RTC reference
}

bool NetworkManager::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool NetworkManager::isNTPSyncNeeded() const {
    return (millis() - _lastNTPSyncTime >= _ntpSyncInterval);
}

// --- Reset NTP Sync Counter ---
void NetworkManager::resetNtpSyncCounter() {
    _lastNTPSyncTime = millis(); // Reset to current time, deferring sync for another interval
    Serial.println("NTP sync counter reset - sync deferred for another interval");
}

// --- Helper methods for captive portal ---

void NetworkManager::_handleRootRequest(WiFiClient client) {
    _sendHttpResponse(client, 200, "text/html", String(CAPTIVE_PORTAL_HTML_FORM));
}

void NetworkManager::_handleSaveRequest(WiFiClient client, String requestLine) {
    // Extract SSID and password from the request
    int ssidStart = requestLine.indexOf("ssid=") + 5;
    int ssidEnd = requestLine.indexOf("&pass=");
    if (ssidEnd == -1) {
        _sendHttpResponse(client, 400, "text/html", "<html><body><h1>Error</h1><p>Invalid form submission</p></body></html>");
        return;
    }
    
    int passStart = ssidEnd + 6;  // "&pass=" is 6 characters
    int passEnd = requestLine.indexOf("&tz=");
    if (passEnd == -1) {
        passEnd = requestLine.indexOf(" HTTP");
    }
    
    String newSsid = _urlDecode(requestLine.substring(ssidStart, ssidEnd));
    String newPass = _urlDecode(requestLine.substring(passStart, passEnd));
    
    Serial.println("Raw SSID: " + newSsid);
    Serial.println("Raw password length: " + String(newPass.length()));
    
    // Test the connection
    if (_testWiFiConnection(newSsid.c_str(), newPass.c_str())) {
        // Save credentials
        saveCredentials(newSsid.c_str(), newPass.c_str());
        _configModeRequired = false;
        
        // Send success page
        _sendHttpResponse(client, 200, "text/html", String(CAPTIVE_PORTAL_HTML_SUCCESS));
    } else {
        // Send failure page
        _sendHttpResponse(client, 200, "text/html", String(CAPTIVE_PORTAL_HTML_FAILED));
    }
} 
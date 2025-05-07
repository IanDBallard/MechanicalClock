#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTC.h>     // Built-in RTC library for UNO R4 WiFi
#include <WiFiS3.h>  // Complete WiFi library for UNO R4 WiFi
#include <Wire.h>
#include <NTPClient.h>
#include <hal_data.h> // Include necessary Renesas headers if not already present

// State management enum
enum ClockState {
    STATE_INIT = 0,
    STATE_CONFIG = 1,
    STATE_CONNECTING_WIFI = 2,
    STATE_SYNCING_TIME = 3,
    STATE_RUNNING = 4,
    STATE_ERROR = 5,
    STATE_POWER_SAVING = 6
};

// Forward declarations
void handleConfigMode();
bool ensureWiFiConnection();
bool updateRTCwithRetry();
void handleNormalOperation();
void updateRTCwithNetworkTime();
void connectToWiFi();
void setupAP();
void IdleStepper();
void WakeStepper();
String urlDecode(String str);
void updateState(ClockState newState);
unsigned long sendCustomNTPpacket(IPAddress& address);
bool calculateDST(const RTCTime& time);

// Global variables and constants first
const unsigned long WIFI_TIMEOUT = 30000;  // 30 seconds timeout for WiFi connection
bool configMode = false;
const char* AP_SSID = "ClockSetup";  // No underscore to avoid encoding issues

// Pin definitions
const int dirPin = 7;
const int stepPin = 8;
const int MS1_PIN = 4;
const int MS2_PIN = 5;
const int MS3_PIN = 6;
const int ENABLE_PIN = 3;
#define LED_PIN 13
#define POWER_PIN 2

// WiFi related
char ssid[32];  // Buffer for WiFi SSID
char pass[64];  // Buffer for WiFi password
int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;
NTPClient timeClient(Udp);
WiFiServer server(80);

// Other constants
const unsigned long STEPPER_IDLE_TIMEOUT = 5000;
const int MAX_NTP_RETRIES = 3;
const unsigned long NTP_RETRY_DELAY = 5000;
const int WIFI_RETRY_COUNT = 3;
const unsigned long WIFI_RETRY_DELAY = 10000;

// State management
ClockState currentState = STATE_INIT;
String lastError = "";

// Stepper configuration
#define motorInterfaceType 1
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);
unsigned long lastStepperMove = 0;

// After the other pin definitions
// Microstepping configuration
#define MICROSTEP_FULL 0b000    // Full step
#define MICROSTEP_HALF 0b100    // 1/2 step
#define MICROSTEP_QUARTER 0b010 // 1/4 step
#define MICROSTEP_EIGHTH 0b110  // 1/8 step
#define MICROSTEP_SIXTEENTH 0b111 // 1/16 step

// Current microstepping mode
const uint8_t CURRENT_MICROSTEP = MICROSTEP_FULL;  // Using full steps for now

// Base steps per revolution (for full stepping)
const int BASE_STEPS_PER_REV = 200;  // For typical NEMA stepper

// Actual steps per revolution based on microstepping
const int stepsPerRevolution = BASE_STEPS_PER_REV * (CURRENT_MICROSTEP == MICROSTEP_FULL ? 1 :
                                                    CURRENT_MICROSTEP == MICROSTEP_HALF ? 2 :
                                                    CURRENT_MICROSTEP == MICROSTEP_QUARTER ? 4 :
                                                    CURRENT_MICROSTEP == MICROSTEP_EIGHTH ? 8 : 16);

// Adjust SecondsPerStep based on microstepping
const int SecondsPerStep = 18 / (CURRENT_MICROSTEP == MICROSTEP_FULL ? 1 :
                                CURRENT_MICROSTEP == MICROSTEP_HALF ? 2 :
                                CURRENT_MICROSTEP == MICROSTEP_QUARTER ? 4 :
                                CURRENT_MICROSTEP == MICROSTEP_EIGHTH ? 8 : 16);

// Change these constants at the top
const unsigned int localPort = 2390;  // Local port for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int CUSTOM_NTP_PACKET_SIZE = 48;  // Renamed from NTP_PACKET_SIZE
byte ntpPacketBuffer[CUSTOM_NTP_PACKET_SIZE];  // Renamed from packetBuffer

// Add with other constants at the top
const int TIME_ZONE_OFFSET = -5;  // EST (Eastern Standard Time)
const bool USE_DST = true;        // Enable automatic DST calculation

// Add with other constants
const unsigned long NTP_SYNC_INTERVAL = 3600000UL;  // 1 hour in milliseconds (from 24 hours)

// Add at the top with other constants
const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
bool isDST = true;  // This will be calculated

// Add at the top with other constants
const char* const DOW_ABBREV[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Add with other global variables near the top
unsigned long lastNTPSync = 0;  // Track last successful NTP sync

// Update the custom character definitions to be non-const
byte WIFI_SYMBOL[] = {
    B00000,
    B00000,
    B00001,
    B00101,
    B10101,
    B00000,
    B00000,
    B00000
};

byte SYNC_SYMBOL[] = {
    B00000,
    B00000,
    B01110,
    B10001,
    B10101,
    B10001,
    B01110,
    B00000
};

// Define base addresses and bit masks based on your provided info
#define SYSTEM_RSTSR0 (*(volatile uint8_t*)0x4001E400)
#define SYSTEM_RSTSR1 (*(volatile uint8_t*)0x4001E401)
#define SYSTEM_RSTSR2 (*(volatile uint8_t*)0x4001E402)

// RSTSR0 Flags (Bit Masks)
#define PORF_BIT    0
#define LVD0RF_BIT  1
#define LVD1RF_BIT  2
#define LVD2RF_BIT  3
#define DPSRSTF_BIT 4

// RSTSR1 Flags (Bit Masks)
#define CWSF_BIT    0

// RSTSR2 Flags (Bit Masks)
#define IWDTRF_BIT  0
#define WDTRF_BIT   1
#define SWRF_BIT    2
// RPERF_BIT 6 and RECCRF_BIT 7 are less relevant for time recovery

// Global variable to store the determined start time
time_t initialClockTime = 0;
bool useEEPROMForInitialTime = false; // Flag to decide time source
bool lcdSuccessfullyInitialized = false; // <<< ADD THIS GLOBAL FLAG

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void connectToWiFi() {
    unsigned long startAttemptTime = millis();
    
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        configMode = true;
        return;
    }

    Serial.println("\n=== Connecting to WiFi ===");
    Serial.print("SSID: '");
    Serial.print(ssid);
    Serial.println("'");
    
    // Add more robust WiFi initialization
    WiFi.end();
    delay(1000);  // Increased delay
    WiFi.disconnect();  // Remove the 'true' parameter
    delay(1000);
    
    int status = WiFi.begin(ssid, pass);
    Serial.print("Initial WiFi status: ");
    Serial.println(status);

    // More detailed connection monitoring
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        delay(1000);  // Give time to stabilize
        printWifiStatus();
    } else {
        Serial.println("\nConnection failed!");
        Serial.print("Final status: ");
        Serial.println(WiFi.status());
        configMode = true;
    }
}
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x27 is the default I2C address, 16x2 display

// EEPROM storage addresses
const int eepromAddressTime = 0;

class LED {
private:
  bool Status;

public:
  byte LEDPin;
  void Initialize(byte Pin);
  void On();
  void Off();
  bool State();
} ArduinoBoardLED;

void LED::Initialize(byte Pin) {
  LEDPin = Pin;
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, LOW);
}

void LED::On() {
  digitalWrite(LEDPin, HIGH);
  Status = true;
}

void LED::Off() {
  digitalWrite(LEDPin, LOW);
  Status = false;
}

bool LED::State() {
  return Status;
}

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Create an RTCTime object to store the current time
RTCTime now;

void IdleStepper() {
  myStepper.disableOutputs();
}

void WakeStepper() {
  myStepper.enableOutputs();
  delay(10);
}

// Interrupt service routine for interrupt 0
void PowerOff() {
  // Get time from onboard RTC (using RTClock class)
  RTC.getTime(now);
  ArduinoBoardLED.On();
  IdleStepper();
  // Store current time in EEPROM
  time_t currentTime = now.getUnixTime();
  // It's generally unsafe to use Serial inside an ISR. Commenting out.
  // Serial.print("PowerOff ISR: Saved time to EEPROM: ");
  // Serial.println(currentTime);
  EEPROM.put(eepromAddressTime, currentTime);
  __WFI(); // Enter low power mode
}

void DisplayDateTime() {
    if (!lcdSuccessfullyInitialized) return; // <<< ADD THIS GUARD

    static int lastDay = -1;
    static int lastHour = -1;
    static int lastMinute = -1;
    static int lastSecond = -1;
    static bool statusBlink = false;
    static unsigned long lastBlink = 0;
    static char timeStr[9] = "00:00:00";
    
    // Get current values
    int currentDay = now.getDayOfMonth();
    int currentHour = now.getHour();
    int currentMinute = now.getMinutes();
    int currentSecond = now.getSeconds();
    
    // Update date only if day changes
    if (lastDay != currentDay) {
        lcd.setCursor(0, 0);
        // Format: "21/Feb/24 Wed" with space for status
        char dateStr[14];  // Shortened to make room for status
        snprintf(dateStr, sizeof(dateStr), "%-02d/%s/%02d %-3s", 
                currentDay,
                MONTH_NAMES[Month2int(now.getMonth()) - 1],
                now.getYear() - 2000,
                DOW_ABBREV[DayOfWeek2int(now.getDayOfWeek(), true) - 1]);
        lcd.print(dateStr);
        lastDay = currentDay;
    }
    
    // Update status indicators every 500ms
    if (millis() - lastBlink >= 500) {
        lastBlink = millis();
        statusBlink = !statusBlink;
        
        // WiFi status - top right corner
        lcd.setCursor(15, 0);
        if (WiFi.status() == WL_CONNECTED) {
            lcd.write(byte(0));  // Solid WiFi symbol
        } else {
            lcd.print(statusBlink ? byte(0) : ' ');  // Blinking WiFi symbol
        }
        
        // NTP sync status - bottom right corner
        lcd.setCursor(15, 1);
        if (millis() - lastNTPSync < NTP_SYNC_INTERVAL * 2) {  // Within 2 intervals
            lcd.write(byte(1));  // Solid sync symbol
        } else {
            lcd.print(statusBlink ? byte(1) : ' ');  // Blinking sync symbol
        }
    }
    
    // Update time display
    if (lastHour != currentHour || lastMinute != currentMinute || lastSecond != currentSecond) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d  ", // Two spaces for padding
                currentHour, currentMinute, currentSecond);
        lcd.setCursor(0, 1);
        lcd.print(timeStr);
        
        lastHour = currentHour;
        lastMinute = currentMinute;
        lastSecond = currentSecond;
    }
}

// Add these constants at the top with your other constants
const long SECONDS_IN_12_HOURS = 43200;  // 12 hours * 60 minutes * 60 seconds

// Structure for storing WiFi credentials
struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool isValid;
};

void loadWiFiCredentials() {
    WiFiCredentials credentials;
    EEPROM.get(100, credentials);
    
    // Add validation check
    if (credentials.isValid && strlen(credentials.ssid) > 0 && strlen(credentials.ssid) < 32) {
        strncpy(ssid, credentials.ssid, 32);
        strncpy(pass, credentials.password, 64);
        Serial.println("Loaded credentials:");
        Serial.println(ssid);
    } else {
        Serial.println("No valid credentials found");
        configMode = true;
    }
}

void saveWiFiCredentials(const char* newSsid, const char* newPassword) {
    WiFiCredentials credentials;
    strncpy(credentials.ssid, newSsid, 32);
    strncpy(credentials.password, newPassword, 64);
    credentials.isValid = true;
    
    EEPROM.put(100, credentials);
}

void setupAP() {
    Serial.println("\n=== Setting up Access Point ===");
    
    // Stop any existing connections and WiFi
    WiFi.end();
    delay(500);
    
    // Check WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        return;
    }
    
    // Print firmware version
    String fv = WiFi.firmwareVersion();
    Serial.print("Firmware version: ");
    Serial.println(fv);
    
    // Configure AP with static IP
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.config(local_ip, gateway, subnet);
    
    // Create AP
    Serial.print("Creating access point named: ");
    Serial.println(AP_SSID);
    
    if (WiFi.beginAP(AP_SSID) != WL_AP_LISTENING) {
        Serial.println("Creating access point failed!");
        return;
    }
    
    // Wait for AP to start
    int timeout = 10;  // 10 second timeout
    while (timeout > 0) {
        if (WiFi.status() == WL_AP_LISTENING) {
            Serial.println("\nAP is now listening!");
            break;
        }
        delay(1000);
        Serial.print(".");
        timeout--;
    }
    
    if (timeout == 0) {
        Serial.println("\nAP failed to start!");
        return;
    }
    
    // Print the AP details
    Serial.println("\n=== AP Setup Complete ===");
    Serial.println("------------------------");
    Serial.print("Network Name: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("------------------------");
    Serial.println("To configure WiFi:");
    Serial.println("1. Connect to '" + String(AP_SSID) + "' network");
    Serial.println("2. Visit http://192.168.4.1");
    Serial.println("------------------------");
    
    // Start web server
    server.begin();
    Serial.println("Web server started");
}

bool testWiFiConnection(const char* testSsid, const char* testPass, WiFiClient& client) {
    Serial.println("\n=== Starting WiFi Test ===");
    Serial.print("Testing SSID: ");
    Serial.println(testSsid);
    
    // Send "Testing connection..." message
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<html><body><h2>Testing WiFi connection...</h2>");
    client.println("<p>Please wait while we test the connection...</p></body></html>");
    client.flush();  // Make sure the message is sent
    
    // Stop AP mode and try to connect
    Serial.println("Stopping AP mode...");
    WiFi.end();
    delay(1000);
    
    Serial.println("Attempting to connect...");
    WiFi.begin(testSsid, testPass);
    
    // Try to connect for 10 seconds
    int attempts = 20;  // 20 * 500ms = 10 seconds
    Serial.print("Connection status: ");
    while (attempts > 0 && WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(WiFi.status());
        Serial.print(".");
        attempts--;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Test connection successful!");
        Serial.print("IP address obtained: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        WiFi.end();  // Disconnect before saving
        Serial.println("=== Test Complete: SUCCESS ===\n");
        return true;
    }
    
    Serial.print("Test connection failed! Final status: ");
    Serial.println(WiFi.status());
    Serial.println("=== Test Complete: FAILED ===\n");
    return false;
}

void handleConfigMode() {
    static unsigned long lastDebug = 0;
    
    if (millis() - lastDebug > 5000) {
        lastDebug = millis();
        Serial.print("Waiting for client connection... IP: ");
        Serial.println(WiFi.localIP());
    }
    
    WiFiClient client = server.available();
    if (client) {
        Serial.println("\n=== Client Connected ===");
        String currentLine = "";
        unsigned long connectionStart = millis();
        
        while (client.connected() && millis() - connectionStart < 5000) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);  // Debug: print all received characters
                
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        // Send the HTML form
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();
                        
                        // Send a proper HTML page with styling
                        client.println("<!DOCTYPE html><html><head>");
                        client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
                        client.println("<title>Clock WiFi Setup</title>");
                        client.println("<style>");
                        client.println("body { font-family: Arial; margin: 20px; }");
                        client.println("input { margin: 10px 0; padding: 5px; width: 200px; }");
                        client.println("</style></head><body>");
                        client.println("<h1>Clock WiFi Setup</h1>");
                        client.println("<form method='get' action='/'>");
                        client.println("<div>SSID:<br><input type='text' name='ssid' required></div>");
                        client.println("<div>Password:<br><input type='password' name='pass' required></div>");
                        client.println("<div><input type='submit' value='Connect'></div>");
                        client.println("</form></body></html>");
                        break;
                    } else {
                        if (currentLine.startsWith("GET /?ssid=")) {
                            // Process form submission
                            Serial.println("\n=== Processing Form Data ===");
                            Serial.println("Raw request: " + currentLine);
                            
                            int ssidStart = currentLine.indexOf("ssid=") + 5;
                            int ssidEnd = currentLine.indexOf("&pass=");
                            if (ssidEnd == -1) {
                                // Send error page
                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-type:text/html");
                                client.println();
                                client.println("<html><body><h1>Error</h1>");
                                client.println("<p>Invalid form submission</p>");
                                client.println("<a href='/'>Try Again</a></body></html>");
                                break;
                            }
                            
                            // Extract credentials
                            int passStart = ssidEnd + 6;  // "&pass=" is 6 characters
                            int passEnd = currentLine.indexOf(" HTTP");
                            
                            String newSsid = currentLine.substring(ssidStart, ssidEnd);
                            String newPass = currentLine.substring(passStart, passEnd);
                            
                            Serial.println("Raw SSID: " + newSsid);
                            Serial.println("Raw password length: " + String(newPass.length()));
                            
                            // URL decode
                            newSsid = urlDecode(newSsid);
                            newPass = urlDecode(newPass);
                            
                            Serial.println("Decoded SSID: " + newSsid);
                            Serial.println("Decoded password length: " + String(newPass.length()));
                            
                            // Test and save the credentials
                            if (testWiFiConnection(newSsid.c_str(), newPass.c_str(), client)) {
                                // Save credentials first
                                saveWiFiCredentials(newSsid.c_str(), newPass.c_str());
                                
                                // Copy credentials to global variables
                                strncpy(ssid, newSsid.c_str(), sizeof(ssid) - 1);
                                strncpy(pass, newPass.c_str(), sizeof(pass) - 1);
                                ssid[sizeof(ssid) - 1] = '\0';  // Ensure null termination
                                pass[sizeof(pass) - 1] = '\0';
                                
                                configMode = false;
                                
                                // Send success page
                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-type:text/html");
                                client.println();
                                client.println("<html><body><h1>Success!</h1>");
                                client.println("<p>WiFi settings saved. Connecting to network...</p></body></html>");
                                
                                // End AP mode and transition to connecting state
                                WiFi.end();
                                delay(1000);
                                updateState(STATE_CONNECTING_WIFI);
                            } else {
                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-type:text/html");
                                client.println();
                                client.println("<html><body><h1>Connection Failed</h1>");
                                client.println("<p>Could not connect to the network. Please try again.</p>");
                                client.println("<a href='/'>Back to Setup</a></body></html>");
                            }
                            break;  // Exit the while loop after processing
                        }
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        
        delay(1);
        client.stop();
        Serial.println("Client disconnected");
    }
}

bool updateRTCwithRetry() {
    Serial.println("\n=== Attempting NTP Time Sync ===");
    
    // Initialize UDP
    Udp.stop();
    if (!Udp.begin(localPort)) {
        Serial.println("Failed to start UDP");
        return false;
    }
    
    for(int i = 0; i < MAX_NTP_RETRIES; i++) {
        Serial.print("NTP attempt ");
        Serial.print(i + 1);
        Serial.print(" of ");
        Serial.println(MAX_NTP_RETRIES);
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi not connected, reconnecting...");
            if (!ensureWiFiConnection()) {
                Serial.println("WiFi reconnection failed!");
                return false;
            }
        }
        
        updateRTCwithNetworkTime();
        
        // Check if time was set by verifying it's not still at the default
        RTCTime currentTime;
        RTC.getTime(currentTime);
        if (currentTime.getYear() > 2000) {
            Serial.println("Time successfully set!");
            return true;
        }
        
        Serial.println("NTP update failed, retrying...");
        delay(NTP_RETRY_DELAY);
    }
    
    Serial.println("All NTP attempts failed!");
    return false;
}

bool ensureWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED) return true;
    
    for(int i = 0; i < WIFI_RETRY_COUNT; i++) {
        connectToWiFi();
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(WIFI_RETRY_DELAY);
    }
    return false;
}

void handleNormalOperation() {
    // Initialize PreviousUnixTime ONLY ONCE using the globally determined time
    static time_t PreviousUnixTime = 0; // Initialize to 0
    static bool firstRun = true;
    if (firstRun) {
         PreviousUnixTime = initialClockTime; // Set using the value from setup()
         Serial.print("[DEBUG] handleNormalOperation starting with initial PreviousUnixTime: "); // DEBUG
         Serial.println(PreviousUnixTime); // DEBUG
         firstRun = false;
         // Optional: If initialClockTime is 0 (RTC init failed), handle it
         if (PreviousUnixTime == 0) {
            Serial.println("[DEBUG] WARNING: Starting time is invalid!"); // DEBUG
            // Perhaps transition to an error state or force NTP sync
         }
    }

    static uint16_t CurrentSecond = 0; // Local tracking, updated from 'now'
    static uint16_t CurrentHour = 0;   // Local tracking, updated from 'now'
    static unsigned long lastDebugPrint = 0; // DEBUG: Throttle debug prints

    int stepsToMove = 0;
    myStepper.run(); // Keep running the stepper state machine

    // Get current time from onboard RTC
    RTC.getTime(now); // 'now' is the global RTCTime object

    // Update LCD display if the second has changed
    if (CurrentSecond != now.getSeconds()) {
        CurrentSecond = now.getSeconds();
        DisplayDateTime(); // Update the LCD

        // --- DEBUG: Print time/stepper info once per second --- 
        if (millis() - lastDebugPrint >= 1000) { // Print roughly every second
            lastDebugPrint = millis();
            Serial.print("[DEBUG] Time: "); Serial.print(now.getUnixTime());
            Serial.print(" | PrevUnix: "); Serial.print(PreviousUnixTime);
            Serial.print(" | SecsPerStep: "); Serial.print(SecondsPerStep);
            Serial.print(" | Target: "); Serial.print(PreviousUnixTime + SecondsPerStep);
            bool shouldMove = (now.getUnixTime() >= PreviousUnixTime + SecondsPerStep);
            Serial.print(" | ShouldMove: "); Serial.print(shouldMove ? "YES" : "NO");
            Serial.print(" | DistToGo: "); Serial.println(myStepper.distanceToGo());
        }
        // --- END DEBUG --- 
    }

    // Check if it's time to move the stepper motor
    // Use >= comparison for safety in case a step interval is missed
    if (now.getUnixTime() >= PreviousUnixTime + SecondsPerStep) {
        long rawTimeDiff = (now.getUnixTime() - PreviousUnixTime);

        // For catch-up, calculate steps based on the difference modulo 12 hours.
        long effectiveTimeDiffForSteps = rawTimeDiff;
        if (rawTimeDiff >= SECONDS_IN_12_HOURS) { // If difference is 12 hours or more
            effectiveTimeDiffForSteps = rawTimeDiff % SECONDS_IN_12_HOURS;
            // If modulo is 0, it means it's an exact multiple of 12 hours, so treat as full 12-hour movement.
            if (effectiveTimeDiffForSteps == 0) {
                effectiveTimeDiffForSteps = SECONDS_IN_12_HOURS;
            }
            Serial.print("[DEBUG] Raw timeDiff >= 12 hours ("); Serial.print(rawTimeDiff);
            Serial.print("s), effectiveTimeDiff for steps: "); Serial.println(effectiveTimeDiffForSteps);
        } else if (rawTimeDiff < 0) {
            // Time went backwards - this is unusual. For now, don't move, just resync PreviousUnixTime.
            Serial.print("[DEBUG] Time went backwards! Raw timeDiff: "); Serial.println(rawTimeDiff);
            effectiveTimeDiffForSteps = 0; 
        }
        // If rawTimeDiff is positive but less than 12 hours, effectiveTimeDiffForSteps is just rawTimeDiff.
        
        stepsToMove = effectiveTimeDiffForSteps / SecondsPerStep;

        if (stepsToMove > 1) {
            Serial.print("[DEBUG] Catch-up MOVE! Raw Diff: "); Serial.print(rawTimeDiff);
            Serial.print("s, Effective Diff for Steps: "); Serial.print(effectiveTimeDiffForSteps);
            Serial.print("s, calculated stepsToMove: "); Serial.println(stepsToMove);
        } else if (stepsToMove == 1) {
             Serial.print("[DEBUG] Regular MOVE. Raw Diff: "); Serial.print(rawTimeDiff);
             Serial.print("s, stepsToMove: "); Serial.println(stepsToMove);
        }

        if (stepsToMove > 0) {
             long targetPosition = stepsToMove + myStepper.distanceToGo(); 
             Serial.print(", Current DistToGo: "); Serial.print(myStepper.distanceToGo()); 
             Serial.print(", Target Pos: "); Serial.println(targetPosition); 
             myStepper.move(targetPosition);

             // After any move, align PreviousUnixTime to be just behind the current time, 
             // on a proper step boundary, to prepare for the next single step.
             // This avoids PreviousUnixTime lapping now.getUnixTime() in a 12-hour cycle.
             PreviousUnixTime = now.getUnixTime() - (now.getUnixTime() % SecondsPerStep);
             // If now.getUnixTime() was exactly on a SecondsPerStep boundary, the above would make
             // PreviousUnixTime == now.getUnixTime(). We want PreviousUnixTime to be *before* the next step.
             // So, if they are equal, subtract one more SecondsPerStep, unless it was a catch-up from a past value.
             if (PreviousUnixTime == now.getUnixTime() && rawTimeDiff >= SecondsPerStep) {
                PreviousUnixTime -= SecondsPerStep;
             }

             Serial.print("[DEBUG] Updated PreviousUnixTime to: "); Serial.println(PreviousUnixTime);
        } else if (rawTimeDiff < 0) {
            // If time went backwards and no steps were moved, still resync PreviousUnixTime.
            PreviousUnixTime = now.getUnixTime() - (now.getUnixTime() % SecondsPerStep);
            Serial.print("[DEBUG] Resynced PreviousUnixTime due to backward time jump: "); Serial.println(PreviousUnixTime);
        }
    }

    // LED indicator based on stepper movement
    if (myStepper.distanceToGo() == 0) {
        ArduinoBoardLED.Off();
    } else {
        ArduinoBoardLED.On();
    }

    // Power management for stepper - NOTE: ENABLE_PIN is assumed externally LOW (always enabled)
    // Calls to WakeStepper/IdleStepper might affect library behavior but not driver power.
    if (myStepper.distanceToGo() != 0) {
        // Stepper needs to move, ensure AccelStepper internal state is active if needed
        // WakeStepper(); // Re-enable library step generation if disableOutputs() was called
        lastStepperMove = millis(); // Update last move time
    } else if (millis() - lastStepperMove > STEPPER_IDLE_TIMEOUT) {
        // Stepper is idle, potentially disable AccelStepper internal step generation
        // IdleStepper(); 
        // NOTE: Since driver is always enabled, this might not save significant power
    }

    // Periodic checks (e.g., DST, NTP sync) - maybe less frequently than every loop
    static unsigned long lastHourlyCheck = 0;
    if (millis() - lastHourlyCheck >= 3600000UL) { // Check roughly every hour
        lastHourlyCheck = millis();
        isDST = calculateDST(now); // Recalculate DST
        Serial.print("Hourly check: DST active = "); Serial.println(isDST);

        // Consider triggering NTP sync here if needed (e.g., if last sync is old)
        if (millis() - lastNTPSync > NTP_SYNC_INTERVAL) {
             Serial.println("NTP sync interval passed. Transitioning to sync time.");
             updateState(STATE_SYNCING_TIME); // This will trigger the sync process
             return; // Exit handleNormalOperation early to allow state transition
        }
    }
    // Update CurrentHour tracking for the hourly check logic
    if (CurrentHour != now.getHour()) {
         CurrentHour = now.getHour();
    }
}

void updateState(ClockState newState) {
    if (currentState == newState) {
        return;
    }
    
    Serial.print("State change: ");
    Serial.print(currentState);
    Serial.print(" -> ");
    Serial.println(newState);
    
    // Handle state exit actions
    switch (currentState) {
        case STATE_RUNNING:
            IdleStepper();
            break;
    }
    
    currentState = newState;
    
    // Handle state entry actions
    switch (currentState) {
        case STATE_CONFIG:
            Serial.println("Starting AP setup...");
            setupAP();  // Always setup AP when entering CONFIG state
            break;
            
        case STATE_RUNNING:
            WakeStepper();
            ArduinoBoardLED.Off();
            break;
            
        case STATE_ERROR:
            ArduinoBoardLED.On();
            break;
    }
}

void setup() {
    // --- Reset Cause Detection ---
    // Read reset status registers IMMEDIATELY (reading clears flags)
    uint8_t rstsr0_val = SYSTEM_RSTSR0;
    uint8_t rstsr1_val = SYSTEM_RSTSR1;
    uint8_t rstsr2_val = SYSTEM_RSTSR2;

    Serial.begin(9600); // Start serial early for debugging reset cause
    Serial.println("\n\n=== Starting Clock Setup ===");
    Serial.print("RSTSR0: 0b"); Serial.println(rstsr0_val, BIN);
    Serial.print("RSTSR1: 0b"); Serial.println(rstsr1_val, BIN);
    Serial.print("RSTSR2: 0b"); Serial.println(rstsr2_val, BIN);

    // Determine reset cause
    bool powerRelatedReset = bitRead(rstsr0_val, PORF_BIT)  ||
                             bitRead(rstsr0_val, LVD0RF_BIT) ||
                             bitRead(rstsr0_val, LVD1RF_BIT) ||
                             bitRead(rstsr0_val, LVD2RF_BIT) ||
                             bitRead(rstsr0_val, DPSRSTF_BIT);

    bool softOrWatchdogReset = bitRead(rstsr2_val, SWRF_BIT)   ||
                               bitRead(rstsr2_val, IWDTRF_BIT) ||
                               bitRead(rstsr2_val, WDTRF_BIT);

    bool isWarmStart = bitRead(rstsr1_val, CWSF_BIT);

    if (powerRelatedReset) {
        Serial.println("Reset Cause: Power-On / Low Voltage / Deep Sleep Exit.");
        useEEPROMForInitialTime = true;
    } else if (softOrWatchdogReset) {
        Serial.println("Reset Cause: Software / Watchdog Reset.");
        useEEPROMForInitialTime = false; // Use current RTC time
    } else if (isWarmStart) {
        // If it's a Warm Start but NOT power-related or known soft/watchdog,
        // infer it's an External Reset (RES pin, Upload, Debug)
        Serial.println("Reset Cause: External Reset (Pin/Upload/Debug).");
        useEEPROMForInitialTime = false; // Use current RTC time
    } else {
        // Default case (e.g., Cold Start without specific power flags - unlikely but possible first boot)
        Serial.println("Reset Cause: Unknown or Initial Cold Boot.");
        useEEPROMForInitialTime = false; // Default to using RTC time
    }

    // --- End Reset Cause Detection ---

    // Initialize hardware AFTER determining reset cause
    Serial.println("Initializing hardware...");
    Wire.begin();
    Serial.println("Wire begun");

    // !!! Initialize RTC *before* trying to get current time !!!
    if (!RTC.begin()) {
        Serial.println("RTC begin failed!");
        // Handle error - maybe force config mode or NTP sync
        // For now, we'll try to continue but time might be wrong
        initialClockTime = 0; // Indicate time is invalid
    } else {
        Serial.println("RTC initialized");

        // Determine the initial time *after* RTC is initialized
        if (useEEPROMForInitialTime) {
            EEPROM.get(eepromAddressTime, initialClockTime);
            Serial.print("Initial time source: EEPROM -> ");
            // Add a simple validity check for EEPROM time
            if (initialClockTime < 1672531200) { // Check if time seems reasonable (e.g., after Jan 1 2023)
                 Serial.print("EEPROM time invalid/unset (");
                 Serial.print(initialClockTime);
                 Serial.println("), using current RTC time.");
                 RTCTime tempNow;
                 RTC.getTime(tempNow); // Get current time from RTC
                 initialClockTime = tempNow.getUnixTime();
            } else {
                 Serial.println(initialClockTime);
            }
        } else {
            RTCTime tempNow;
            RTC.getTime(tempNow); // Get current time from RTC
            initialClockTime = tempNow.getUnixTime();
            Serial.print("Initial time source: Current RTC -> ");
            Serial.println(initialClockTime);
        }
    }

    // A4988 pin setup
    pinMode(MS1_PIN, OUTPUT);
    pinMode(MS2_PIN, OUTPUT);
    pinMode(MS3_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    Serial.println("Stepper pins configured");

    // Set microstepping mode
    digitalWrite(MS1_PIN, (CURRENT_MICROSTEP & 0b100) ? HIGH : LOW);
    digitalWrite(MS2_PIN, (CURRENT_MICROSTEP & 0b010) ? HIGH : LOW);
    digitalWrite(MS3_PIN, (CURRENT_MICROSTEP & 0b001) ? HIGH : LOW);
    digitalWrite(ENABLE_PIN, HIGH);  // Start with stepper disabled
    delay(100);
    digitalWrite(ENABLE_PIN, LOW);   // Then enable it
    Serial.println("Microstepping configured");

    pinMode(POWER_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(POWER_PIN), PowerOff, FALLING);
    Serial.println("Power interrupt configured");

    myStepper.setMaxSpeed(50);
    myStepper.setAcceleration(2);
    Serial.println("Stepper parameters set");

    Serial.println("Initializing LCD...");
    Wire.beginTransmission(0x27); 
    byte error = Wire.endTransmission();
    byte lcd_address = 0x27; 

    if (error != 0) {
        Serial.println("LCD not found at 0x27, trying 0x3F...");
        Wire.beginTransmission(0x3F);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.println("LCD found at address 0x3F");
            lcd_address = 0x3F;
            // It's important to re-initialize the lcd object if the address changed
            // However, the global lcd object is already created with 0x27.
            // For robust dynamic address handling, lcd would ideally be a pointer initialized here.
            // For now, if 0x27 fails and 0x3F works, the global lcd object is for the wrong address.
            // This requires a more significant change if 0x3F is commonly used.
            // For this fix, we assume if error is 0, the *globally defined* lcd object is the one to use.
            // If your LCD is consistently at 0x3F, change the global definition: LiquidCrystal_I2C lcd(0x3F, 16, 2);
        } else {
            Serial.println("LCD not found! Check connections");
        }
    } else {
         Serial.println("LCD found at address " + String(lcd_address, HEX));
    }

    if (error == 0) { 
        delay(100);
        Serial.println("Starting LCD init...");
        lcd.init(); 
        lcd.clear();
        lcd.backlight();
        lcdSuccessfullyInitialized = true; // <<< SET FLAG HERE
        Serial.println("LCD init done");

        Serial.println("Clearing display...");
        lcd.clear();
        Serial.println("Display cleared");

        Serial.println("Writing test message...");
        lcd.setCursor(0, 0);
        lcd.print("Clock Init...");
        Serial.println("Test message written");

        byte wifiChar[8], syncChar[8];
        memcpy(wifiChar, WIFI_SYMBOL, 8);
        memcpy(syncChar, SYNC_SYMBOL, 8);
        lcd.createChar(0, wifiChar);
        lcd.createChar(1, syncChar);
    } else {
        lcdSuccessfullyInitialized = false; // <<< ENSURE FLAG IS FALSE IF NOT FOUND/INIT
    }
    // --- End LCD Init block ---


    Serial.println("Loading WiFi credentials...");
    loadWiFiCredentials();

    if (configMode) {
        Serial.println("\n=== Entering Configuration Mode ===");
        // Set initial state later in setup
    } else {
        Serial.println("\n=== Starting Normal Operation ===");
        // RTC already initialized above

        ArduinoBoardLED.Initialize(LED_PIN);
        Serial.println("LED initialized");

        if (!RTC.isRunning()) {
            Serial.println("RTC was not running! Time may be incorrect until NTP sync.");
            // Consider forcing NTP sync or entering error state if RTC isn't running
        }
    }

    // Update initial state based on configMode
    updateState(configMode ? STATE_CONFIG : STATE_CONNECTING_WIFI); // Set initial state here


    Serial.println("Setup complete, entering main loop");
}

void loop() {
    static unsigned long lastDebug = 0;
    
    // Print state every 5 seconds
    if (millis() - lastDebug > 5000) {
        lastDebug = millis();
        Serial.print("\nCurrent state: ");
        Serial.println(currentState);
    }
    
    switch (currentState) {
        case STATE_INIT:
            updateState(configMode ? STATE_CONFIG : STATE_CONNECTING_WIFI);
            break;
            
        case STATE_CONFIG:
            handleConfigMode();
            break;
            
        case STATE_CONNECTING_WIFI:
            if (ensureWiFiConnection()) {
                updateState(STATE_SYNCING_TIME);
            } else {
                lastError = "WiFi connection failed";
                updateState(STATE_ERROR);
            }
            break;
            
        case STATE_SYNCING_TIME:
            if (updateRTCwithRetry()) {
                updateState(STATE_RUNNING);
            } else {
                lastError = "NTP sync failed";
                updateState(STATE_ERROR);
            }
            break;
            
        case STATE_RUNNING:
            handleNormalOperation();
            break;
            
        case STATE_ERROR:
            if (lcdSuccessfullyInitialized) { // <<< ADD THIS GUARD
                lcd.clear();
                lcd.print("Error:");
                lcd.setCursor(0, 1);
                lcd.print(lastError);
            }
            Serial.println("Error State: " + lastError); // Print error to serial as well
            delay(5000);
            updateState(STATE_INIT);
            break;
    }
}

String urlDecode(String str) {
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
            sscanf(str.substring(i + 1, i + 3).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i += 2;
        }
    }
    return ret;
}

void updateRTCwithNetworkTime() {
    Serial.println("\n=== Updating RTC from NTP ===");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected for NTP!");
        return;
    }
    
    // Multiple NTP attempts
    for (int attempt = 0; attempt < 3; attempt++) {
        Serial.print("NTP attempt ");
        Serial.println(attempt + 1);
        
        sendCustomNTPpacket(timeServer);
        
        unsigned long startWait = millis();
        while (millis() - startWait < 2000) {  // Wait up to 2 seconds for response
            if (Udp.parsePacket()) {
                Serial.println("Received NTP response");
                Udp.read(ntpPacketBuffer, CUSTOM_NTP_PACKET_SIZE);  // Updated buffer name
                
                // Convert NTP time (seconds since Jan 1 1900) to Unix time
                unsigned long highWord = word(ntpPacketBuffer[40], ntpPacketBuffer[41]);
                unsigned long lowWord = word(ntpPacketBuffer[42], ntpPacketBuffer[43]);
                unsigned long secsSince1900 = highWord << 16 | lowWord;
                const unsigned long seventyYears = 2208988800UL;
                unsigned long epoch = secsSince1900 - seventyYears;
                
                // Calculate if we're in DST period before applying any offsets
                RTCTime tempTime = RTCTime(epoch + TIME_ZONE_OFFSET * 3600);
                isDST = USE_DST && calculateDST(tempTime);
                
                // Apply timezone and DST adjustments
                epoch += TIME_ZONE_OFFSET * 3600;  // Apply EST offset first
                if (isDST) {
                    epoch += 3600;  // Add DST hour if needed
                    Serial.println("DST is active, adding 1 hour");
                }
                
                RTCTime timeToSet = RTCTime(epoch);
                RTC.setTime(timeToSet);
                lastNTPSync = millis();  // Update the sync timestamp
                
                // Debug output
                Serial.print("Time zone offset: ");
                Serial.print(TIME_ZONE_OFFSET);
                Serial.println(" hours");
                Serial.print("DST active: ");
                Serial.println(isDST ? "Yes" : "No");
                Serial.println("Updated RTC time: " + String(timeToSet));
                return;
            }
            delay(10);
        }
        
        Serial.println("NTP attempt failed, retrying...");
        delay(1000);
    }
    
    Serial.println("All NTP attempts failed!");
}

unsigned long sendCustomNTPpacket(IPAddress& address) {
    Serial.println("Sending NTP request...");
    
    // Initialize the packet buffer
    memset(ntpPacketBuffer, 0, CUSTOM_NTP_PACKET_SIZE);
    
    // Set NTP request headers
    ntpPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
    ntpPacketBuffer[1] = 0;     // Stratum
    ntpPacketBuffer[2] = 6;     // Polling Interval
    ntpPacketBuffer[3] = 0xEC;  // Peer Clock Precision
    
    // Send the packet
    Udp.beginPacket(address, 123); // NTP port is 123
    Udp.write(ntpPacketBuffer, CUSTOM_NTP_PACKET_SIZE);
    return Udp.endPacket();
}

// Update the calculateDST function to use Month2int
bool calculateDST(const RTCTime& time) {
    int month = Month2int(time.getMonth());  // Convert Month to int
    int day = time.getDayOfMonth();
    int dow = DayOfWeek2int(time.getDayOfWeek(), true);  // 1 = Sunday, 7 = Saturday
    
    // US DST Rules:
    // Begins second Sunday in March
    // Ends first Sunday in November
    
    if (month < 3 || month > 11) return false;  // Jan, Feb, Dec
    if (month > 3 && month < 11) return true;   // Apr-Oct
    
    int dowInMonth = (day - 1) / 7 + 1;  // Calculate which occurrence of DOW
    
    if (month == 3) {  // March
        return (dowInMonth >= 2 && dow == 1) || (day >= 14);  // After second Sunday
    } else {  // November
        return !(dowInMonth >= 1 && dow == 1) && (day <= 7);  // Before first Sunday
    }
}
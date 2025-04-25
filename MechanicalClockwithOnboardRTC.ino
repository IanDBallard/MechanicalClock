#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTC.h>     // RTC library for DS3231/DS1307
#include <WiFi.h>       // Standard WiFi library
#include <WiFiUdp.h>
#include <Wire.h>
#include <NTPClient.h>
// Forward declarations
void updateRTCwithNetworkTime();
void connectToWiFi();
void setupAP();
void IdleStepper();
void WakeStepper();

// Global variables and constants first
const unsigned long WIFI_TIMEOUT = 30000;  // 30 seconds timeout for WiFi connection
bool configMode = false;
const char* AP_SSID = "Clock_Setup";
const char* AP_PASSWORD = "admin";

// Pin definitions
const int dirPin = 7;
const int stepPin = 8;
const int MS1_PIN = 4;
const int MS2_PIN = 5;
const int MS3_PIN = 6;
const int ENABLE_PIN = 3;
#define SDA_PIN A4
#define SCL_PIN A5
#define LED_PIN 13
#define POWER_PIN 2

// WiFi related
char ssid[] = "Chez Bell";
char pass[] = "Tobias01";
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
enum ClockState {
    STATE_INIT,
    STATE_CONFIG,
    STATE_CONNECTING_WIFI,
    STATE_SYNCING_TIME,
    STATE_RUNNING,
    STATE_ERROR,
    STATE_POWER_SAVING
};
ClockState currentState = STATE_INIT;
String lastError = "";

// Stepper configuration
#define motorInterfaceType 1
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);
unsigned long lastStepperMove = 0;

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

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttemptTime > WIFI_TIMEOUT) {
            Serial.println("Failed to connect to WiFi");
            configMode = true;
            return;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected to WiFi");
    printWifiStatus();
}
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address

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
  EEPROM.put(eepromAddressTime, now.getUnixTime());
  __WFI();
}

void DisplayDateTime() {
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print(now.getYear() - 2000, DEC);
  lcd.print('/');
  lcd.print(Month2int(now.getMonth()), DEC);
  lcd.print('/');
  lcd.print(now.getDayOfMonth(), DEC);
  lcd.print(" ");
  lcd.print(daysOfTheWeek[DayOfWeek2int(now.getDayOfWeek(), true) - 1]);
  lcd.setCursor(0, 1);
  lcd.print(now.getHour(), DEC);
  lcd.print(':');
  lcd.print(now.getMinutes(), DEC);
  lcd.print(':');
  lcd.print(now.getSeconds(), DEC);
}

time_t GetPowerDownTime() {
  time_t storedUnixTime;

  // Retrieve last known position of the clock hands and move them to reflect current time.
  // Get time from onboard RTC (using RTClock class)
  RTC.getTime(now);
  // Read last stored time from EEPROM
  EEPROM.get(eepromAddressTime, storedUnixTime);
  ;
  Serial.print(" Stored Unix Time : ");
  Serial.println(storedUnixTime);
  return storedUnixTime;
}

// Add with other constants
const int TIME_ZONE_OFFSET = -5;  // EST, adjust as needed
const int DAYLIGHT_SAVING = 1;    // Set to 1 during DST, 0 otherwise

void updateRTCwithNetworkTime() {
  RTCTime currentTime;

  RTC.getTime(currentTime);
  Serial.println("The RTC preupdate time is: " + String(currentTime));

  if (!timeClient.update()) {
    Serial.println("Failed to get time from NTP server");
    return;
  }

  // Get the current date and time from an NTP server and convert
  // it to UTC +2 by passing the time zone offset in hours.
  // You may change the time zone offset to your local one.
  auto timeZoneOffsetHours = TIME_ZONE_OFFSET + DAYLIGHT_SAVING;
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);

  // Retrieve the date and time from the RTC and print them

  RTC.getTime(currentTime);
  Serial.println("The RTC was just set to: " + String(currentTime));
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
    
    if (credentials.isValid) {
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
    WiFi.beginAP(AP_SSID, AP_PASSWORD);
    
    Serial.println("Access Point Mode Started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();
}

// After the pin definitions, add back these constants
// Define motor interface type
#define motorInterfaceType 1

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

long int CumHourlySteps;

// Add this function to handle state transitions
void updateState(ClockState newState) {
    if (currentState == newState) return;
    
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
            setupAP();
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

// Modify loop() to use states
void loop() {
    switch (currentState) {
        case STATE_INIT:
            if (!RTC.isRunning()) {
                lastError = "RTC not running";
                updateState(STATE_ERROR);
                return;
            }
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
            // Display error on LCD
            lcd.clear();
            lcd.print("Error:");
            lcd.setCursor(0, 1);
            lcd.print(lastError);
            delay(5000);
            // Attempt to recover based on last state
            updateState(STATE_INIT);
            break;
    }
}

// Break out normal operation into its own function
void handleNormalOperation() {
    // Original loop code for normal operation goes here
    static time_t PreviousUnixTime = GetPowerDownTime();
    static uint16_t CurrentSecond = 0;
    static uint16_t CurrentHour = 0;
    
    int stepsToMove = 0;
    myStepper.run();

    // Get time from onboard RTC (using RTClock class)
    RTC.getTime(now);

    if (CurrentSecond != now.getSeconds()) {
        CurrentSecond = now.getSeconds();
        DisplayDateTime();
    }

    if ((now.getUnixTime() % SecondsPerStep == 0) && (now.getUnixTime() != PreviousUnixTime)) {
        // Calculate time difference and limit to 12 hours
        long timeDiff = (now.getUnixTime() - PreviousUnixTime) % SECONDS_IN_12_HOURS;
        stepsToMove = timeDiff / SecondsPerStep;
        
        // Add some debug output
        Serial.print("Raw time difference (seconds): ");
        Serial.println(now.getUnixTime() - PreviousUnixTime);
        Serial.print("Limited time difference (seconds): ");
        Serial.println(timeDiff);
        Serial.print("Steps to move: ");
        Serial.println(stepsToMove);
        
        myStepper.move(stepsToMove + myStepper.distanceToGo());
        PreviousUnixTime = now.getUnixTime();
    }
    if (/*(now.getHour() == 0) && */ (CurrentHour != now.getHour())) {  ///Update RTC clock with network time at midnight
        CurrentHour = now.getHour();
        updateRTCwithNetworkTime();
    }
    if (myStepper.distanceToGo() == 0) {
        ArduinoBoardLED.Off();
    } else {
        ArduinoBoardLED.On();
    }

    if (myStepper.distanceToGo() != 0) {
        lastStepperMove = millis();
        WakeStepper();
    } else if (millis() - lastStepperMove > STEPPER_IDLE_TIMEOUT) {
        IdleStepper();  // Power down stepper when not moving
    }
}

// Handle config mode in its own function
void handleConfigMode() {
    WiFiClient client = server.available();
    if (client) {
        // Handle client connection
        client.stop();
    }
}

bool updateRTCwithRetry() {
    for(int i = 0; i < MAX_NTP_RETRIES; i++) {
        if (timeClient.update()) {
            updateRTCwithNetworkTime();
            return true;
        }
        delay(NTP_RETRY_DELAY);
    }
    return false;
}

class EEPROMManager {
    static const int BUFFER_SIZE = 32;  // Circular buffer size
    static const int START_ADDRESS = 0;
    int currentIndex = 0;

public:
    void saveTime(time_t value) {
        int address = START_ADDRESS + (currentIndex * sizeof(time_t));
        EEPROM.put(address, value);
        currentIndex = (currentIndex + 1) % BUFFER_SIZE;
        EEPROM.put(START_ADDRESS + (BUFFER_SIZE * sizeof(time_t)), currentIndex);
    }

    time_t loadTime() {
        EEPROM.get(START_ADDRESS + (BUFFER_SIZE * sizeof(time_t)), currentIndex);
        currentIndex = (currentIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE;
        time_t value;
        EEPROM.get(START_ADDRESS + (currentIndex * sizeof(time_t)), value);
        return value;
    }
};

bool ensureWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED) return true;
    
    for(int i = 0; i < WIFI_RETRY_COUNT; i++) {
        connectToWiFi();
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(WIFI_RETRY_DELAY);
    }
    return false;
}

void setup() {
    // Add this at the very start of setup()
    delay(1000);  // Give the board time to stabilize
    
    // Initialize I2C
    Wire.begin();  // UNO R4 uses default I2C pins
    
    // A4988 pin setup
    pinMode(MS1_PIN, OUTPUT);
    pinMode(MS2_PIN, OUTPUT);
    pinMode(MS3_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    
    // Set microstepping mode
    digitalWrite(MS1_PIN, (CURRENT_MICROSTEP & 0b100) ? HIGH : LOW);
    digitalWrite(MS2_PIN, (CURRENT_MICROSTEP & 0b010) ? HIGH : LOW);
    digitalWrite(MS3_PIN, (CURRENT_MICROSTEP & 0b001) ? HIGH : LOW);
    
    // Enable the driver (active LOW)
    digitalWrite(ENABLE_PIN, LOW);
    
    Serial.begin(9600);
    pinMode(POWER_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(POWER_PIN), PowerOff, FALLING);
    myStepper.setMaxSpeed(100);
    myStepper.setAcceleration(5);
    //myStepper.setSpeed(5);
    lcd.init();
    lcd.clear();
    lcd.backlight();  // Make sure backlight is on
    Serial.begin(9600);
    while (!Serial)
      ;

    loadWiFiCredentials();
    if (!configMode) {
        connectToWiFi();
    }
    
    if (configMode) {
        Serial.println("Entering configuration mode");
        setupAP();
    } else {
        RTC.begin();
        Serial.println("\nStarting connection to NTP server...");
        timeClient.begin();
        updateRTCwithNetworkTime();
        ArduinoBoardLED.Initialize(LED_PIN);
        
        if (!RTC.isRunning()) {
            Serial.println("RTC was not running! Setting time from NTP...");
            updateRTCwithNetworkTime();
        }
    }
}
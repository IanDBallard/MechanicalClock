#ifndef TEST_FRAMEWORK_DESKTOP_H
#define TEST_FRAMEWORK_DESKTOP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <functional>

// Mock Arduino environment for desktop testing
namespace Arduino {
    // Mock Arduino types and functions
    typedef unsigned char byte;
    typedef unsigned long uint32_t;
    typedef unsigned int uint16_t;
    typedef unsigned char uint8_t;
    typedef signed long int32_t;
    typedef signed int int16_t;
    typedef signed char int8_t;
    
    // Mock Arduino constants
    const int HIGH = 1;
    const int LOW = 0;
    const int INPUT = 0;
    const int OUTPUT = 1;
    const int INPUT_PULLUP = 2;
    
    // Mock Arduino functions
    void digitalWrite(int pin, int state);
    int digitalRead(int pin);
    void pinMode(int pin, int mode);
    void delay(unsigned long ms);
    unsigned long millis();
    
    // Mock Serial
    class SerialClass {
    public:
        void begin(int baud);
        void print(const char* str);
        void print(int value);
        void print(unsigned long value);
        void println(const char* str);
        void println(int value);
        void println(unsigned long value);
        bool available();
        int read();
    };
    extern SerialClass Serial;
}

// Mock WiFi types
namespace WiFiS3 {
    const int WL_DISCONNECTED = 0;
    const int WL_CONNECTED = 1;
    const int WL_CONNECT_FAILED = 2;
    const int WL_AP_LISTENING = 3;
    
    class WiFiClass {
    public:
        int status();
        bool begin(const char* ssid, const char* password);
        void disconnect();
        void end();
        IPAddress localIP();
        bool beginAP(const char* ssid);
        void endAP();
    };
    extern WiFiClass WiFi;
}

// Mock RTC types
namespace RTC {
    class RTCTime {
    public:
        RTCTime();
        RTCTime(int day, int month, int year, int hour, int minute, int second);
        unsigned long getUnixTime() const;
        int getDay() const;
        int getMonth() const;
        int getYear() const;
        int getHour() const;
        int getMinute() const;
        int getSecond() const;
    };
    
    class RTClock {
    public:
        bool begin();
        void getTime(RTCTime& time);
        void setTime(const RTCTime& time);
    };
    extern RTClock RTC;
}

// Mock EEPROM
namespace EEPROM {
    template<typename T>
    void put(int address, const T& data);
    
    template<typename T>
    void get(int address, T& data);
}

// Mock Wire (I2C)
namespace Wire {
    void begin();
    void beginTransmission(int address);
    int endTransmission();
    int requestFrom(int address, int quantity);
    int available();
    int read();
    int write(uint8_t data);
}

// Mock IPAddress
class IPAddress {
public:
    IPAddress();
    IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth);
    uint8_t operator[](int index) const;
};

// Mock WiFiClient
class WiFiClient {
public:
    bool connected();
    int available();
    int read();
    int write(uint8_t data);
    void stop();
    void print(const char* str);
    void println(const char* str);
};

// Mock WiFiServer
class WiFiServer {
public:
    WiFiServer(int port);
    void begin();
    WiFiClient available();
    void stop();
};

// Mock WiFiUDP
class WiFiUDP {
public:
    int beginPacket(IPAddress ip, int port);
    int write(uint8_t* buffer, int size);
    int endPacket();
    int parsePacket();
    int read(uint8_t* buffer, int size);
    int begin(int port);
    void stop();
};

// Test utilities for desktop
class TestUtils {
public:
    static void resetMocks();
    static void setupMockWiFi(int status);
    static void setupMockRTC(const RTC::RTCTime& time);
    static void setupMockEEPROM();
};

#endif // TEST_FRAMEWORK_DESKTOP_H 
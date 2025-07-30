// Simple WiFi UDP Logger for Mechanical Clock
// Add this to your existing code for connectionless logging

#include <WiFiUdp.h>

class WiFiLogger {
private:
    WiFiUDP _udp;
    IPAddress _logServerIP;
    int _logPort;
    bool _enabled;
    
public:
    WiFiLogger() : _logPort(8888), _enabled(false) {}
    
    bool begin(IPAddress serverIP, int port = 8888) {
        _logServerIP = serverIP;
        _logPort = port;
        _enabled = _udp.begin(port);
        return _enabled;
    }
    
    void log(String message) {
        if (!_enabled || WiFi.status() != WL_CONNECTED) return;
        
        String logMessage = "[CLOCK " + String(millis()) + "ms] " + message;
        _udp.beginPacket(_logServerIP, _logPort);
        _udp.print(logMessage);
        _udp.endPacket();
    }
    
    void logf(const char* format, ...) {
        if (!_enabled) return;
        
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(String(buffer));
    }
    
    void enable() { _enabled = true; }
    void disable() { _enabled = false; }
    bool isEnabled() { return _enabled; }
};

// Global logger instance
WiFiLogger wifiLog;

// In your setup():
void setup() {
    Serial.begin(115200);
    
    // ... your existing WiFi setup ...
    
    // Initialize WiFi logging (replace with your computer's IP)
    IPAddress logServerIP(192, 168, 1, 100); // Your computer's IP
    if (wifiLog.begin(logServerIP)) {
        wifiLog.log("WiFi Logger initialized successfully");
        Serial.println("WiFi logging enabled - check UDP port 8888");
    } else {
        Serial.println("WiFi logging failed to initialize");
    }
    
    // ... rest of your setup ...
}

// Replace Serial.println() calls with wifiLog.log():
void someFunction() {
    // Old way:
    // Serial.println("System initialized");
    
    // New way (logs to both Serial and WiFi):
    String message = "System initialized";
    Serial.println(message);  // Keep for USB debugging
    wifiLog.log(message);     // Add WiFi logging
    
    // Or use formatted logging:
    wifiLog.logf("Temperature: %.2f°C, Humidity: %.1f%%", temp, humidity);
}

// Example integration with your StateManager:
void StateManager::transitionTo(ClockState newState) {
    // ... your existing code ...
    
    // Add WiFi logging:
    wifiLog.logf("State transition: %s -> %s", 
                 stateToString(_currentState), 
                 stateToString(newState));
    
    // ... rest of your code ...
}

// Example NetworkManager integration:
void NetworkManager::connectToWiFi() {
    wifiLog.log("Attempting WiFi connection...");
    
    // ... your connection code ...
    
    if (connected) {
        wifiLog.logf("WiFi connected to %s, IP: %s", 
                     WiFi.SSID().c_str(), 
                     WiFi.localIP().toString().c_str());
    } else {
        wifiLog.log("WiFi connection failed");
    }
}

// Computer-side Python listener (save as wifi_logger.py):
/*
import socket
import datetime

def listen_for_logs():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', 8888))
    print("WiFi Logger listening on port 8888...")
    print("=" * 50)
    
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            timestamp = datetime.datetime.now().strftime("%H:%M:%S")
            message = data.decode('utf-8')
            print(f"[{timestamp}] {addr[0]}: {message}")
        except KeyboardInterrupt:
            print("\nStopping logger...")
            break
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    listen_for_logs()
*/ 
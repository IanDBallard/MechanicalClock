#include "StateManager.h"
#include <Arduino.h>

StateManager::StateManager(NetworkManager& networkManager, LCDDisplay& lcdDisplay, 
                           Clock& clock, RTClock& rtc)
    : _networkManager(networkManager), _lcdDisplay(lcdDisplay), _clock(clock), _rtc(rtc),
      _currentState(STATE_INIT), _lastStateChange(0), _lastDebugPrint(0),
      _configStartTime(0), _wifiConnectStartTime(0), _ntpSyncStartTime(0) {
}

void StateManager::update() {
    // Print state info every 5 minutes
    if (millis() - _lastDebugPrint > 300000UL) {
        _lastDebugPrint = millis();
        printStateInfo();
    }
    
    // Run the current state's logic
    _runCurrentStateLogic();
    
    // Only update the clock in appropriate states (not during config, connecting, etc.)
    if (_currentState == STATE_RUNNING || _currentState == STATE_POWER_SAVING) {
        _clock.update();
    }
}

void StateManager::transitionTo(ClockState newState) {
    if (_currentState == newState) {
        return; // No state change needed
    }
    
    Serial.print("State change: ");
    Serial.print(_currentState);
    Serial.print(" -> ");
    Serial.println(newState);
    
    // Handle state exit actions
    _handleStateExit(_currentState);
    
    // Update state
    ClockState oldState = _currentState;
    _currentState = newState;
    _lastStateChange = millis();
    
    // Handle state entry actions
    _handleStateEntry(newState);
}

ClockState StateManager::getCurrentState() const {
    return _currentState;
}

void StateManager::setLastError(const String& error) {
    _lastError = error;
    Serial.print("Error set: ");
    Serial.println(error);
}

String StateManager::getLastError() const {
    return _lastError;
}

void StateManager::printStateInfo() {
    Serial.print("Current state: ");
    Serial.println(_currentState);
    Serial.print("State duration: ");
    Serial.print((millis() - _lastStateChange) / 1000);
    Serial.println(" seconds");
}

void StateManager::_handleStateEntry(ClockState newState) {
    switch (newState) {
        case STATE_INIT:
            _lcdDisplay.printLine(0, "Initializing...");
            _lcdDisplay.printLine(1, "Please Wait");
            break;
            
        case STATE_CONFIG:
            Serial.println("Starting AP setup...");
            _configStartTime = millis();
            _networkManager.startConfigurationMode();
            _lcdDisplay.printLine(0, "Config Mode");
            _lcdDisplay.printLine(1, "Connect to AP");
            break;
            
        case STATE_CONNECTING_WIFI:
            Serial.println("Attempting WiFi connection...");
            _wifiConnectStartTime = millis();
            _lcdDisplay.printLine(0, "Connecting WiFi");
            _lcdDisplay.printLine(1, "Please Wait...");
            break;
            
        case STATE_SYNCING_TIME:
            Serial.println("Starting NTP sync...");
            _ntpSyncStartTime = millis();
            _lcdDisplay.printLine(0, "Syncing Time");
            _lcdDisplay.printLine(1, "NTP Server...");
            break;
            
        case STATE_RUNNING:
            Serial.println("Entering normal operation...");
            _lcdDisplay.printLine(0, "Clock Running");
            _lcdDisplay.printLine(1, "Normal Mode");
            break;
            
        case STATE_ERROR:
            Serial.print("Entering error state: ");
            Serial.println(_lastError);
            _lcdDisplay.printLine(0, "ERROR:");
            _lcdDisplay.printLine(1, _lastError);
            break;
            
        case STATE_POWER_SAVING:
            Serial.println("Entering power saving mode...");
            _lcdDisplay.printLine(0, "Power Saving");
            _lcdDisplay.printLine(1, "Mode Active");
            break;
    }
}

void StateManager::_handleStateExit(ClockState oldState) {
    switch (oldState) {
        case STATE_CONFIG:
            _networkManager.stopConfigurationMode();
            break;
            
        case STATE_RUNNING:
            // Any cleanup needed when leaving running state
            break;
            
        default:
            // No specific exit actions for other states
            break;
    }
}

void StateManager::_runCurrentStateLogic() {
    switch (_currentState) {
        case STATE_INIT:
            _runInitState();
            break;
            
        case STATE_CONFIG:
            _runConfigState();
            break;
            
        case STATE_CONNECTING_WIFI:
            _runConnectingWiFiState();
            break;
            
        case STATE_SYNCING_TIME:
            _runSyncingTimeState();
            break;
            
        case STATE_RUNNING:
            _runRunningState();
            break;
            
        case STATE_ERROR:
            _runErrorState();
            break;
            
        case STATE_POWER_SAVING:
            _runPowerSavingState();
            break;
    }
}

void StateManager::_runInitState() {
    // Initialization is handled in setup(), so this state should transition quickly
    if (_networkManager.needsConfiguration()) {
        transitionTo(STATE_CONFIG);
    } else {
        transitionTo(STATE_CONNECTING_WIFI);
    }
}

void StateManager::_runConfigState() {
    // Handle configuration mode
    String errorMessage;
    if (_networkManager.handleConfigPortal(errorMessage)) {
        // Configuration successful
        transitionTo(STATE_CONNECTING_WIFI);
    }
    
    // Timeout after 5 minutes in config mode
    if (millis() - _configStartTime > 300000UL) {
        setLastError("Config Timeout");
        transitionTo(STATE_ERROR);
    }
}

void StateManager::_runConnectingWiFiState() {
    // Attempt WiFi connection
    if (_networkManager.connectToWiFi()) {
        transitionTo(STATE_SYNCING_TIME);
    }
    
    // Timeout after 30 seconds
    if (millis() - _wifiConnectStartTime > 30000UL) {
        // Reset NTP sync counter and return to running state
        _networkManager.resetNtpSyncCounter();
        transitionTo(STATE_RUNNING);
    }
}

void StateManager::_runSyncingTimeState() {
    // Attempt NTP sync using the RTC reference
    if (_networkManager.syncTimeWithRTC(_rtc)) {
        // After successful NTP sync, update the clock's current time
        _clock.updateCurrentTime();
        
        transitionTo(STATE_RUNNING);
    }
    
    // Timeout after 30 seconds
    if (millis() - _ntpSyncStartTime > 30000UL) {
        // Reset NTP sync counter and return to running state
        _networkManager.resetNtpSyncCounter();
        transitionTo(STATE_RUNNING);
    }
}

void StateManager::_runRunningState() {
    // Get current UTC time and convert to local for display
    time_t currentUTC = getCurrentUTC();
    RTCTime localTime = convertUTCToLocal(currentUTC, _networkManager.getTimeZoneOffset(), _networkManager.getUseDST());
    

    
    _lcdDisplay.updateTimeAndDate(localTime);
    _lcdDisplay.updateNetworkStatus(_networkManager.getWiFiStatus(), 
                                _networkManager.getLastNtpSyncTime(),
                                _networkManager.getNtpSyncInterval());
    
    // Check if periodic NTP sync is needed
    if (_networkManager.isNTPSyncNeeded()) {
        // Check WiFi status before attempting NTP sync
        if (_networkManager.isWiFiConnected()) {
            transitionTo(STATE_SYNCING_TIME);
        } else {
            // WiFi disconnected, try to reconnect first
            transitionTo(STATE_CONNECTING_WIFI);
        }
    }
}

void StateManager::_runErrorState() {
    // Display error for 5 seconds, then try to recover
    static unsigned long errorStartTime = 0;
    if (errorStartTime == 0) {
        errorStartTime = millis();
    }
    
    if (millis() - errorStartTime > 5000UL) {
        errorStartTime = 0; // Reset for next error
        transitionTo(STATE_INIT); // Try to recover
    }
}

void StateManager::_runPowerSavingState() {
    // Minimal operation in power saving mode
    // The ISR has already saved the time and disabled the stepper
    // This state is mainly for display purposes
    
    // Could implement additional power saving measures here
    // For now, just stay in this state until power is restored
} 
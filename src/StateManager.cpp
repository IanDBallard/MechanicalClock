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
    if (millis() - _lastDebugPrint > DEBUG_PRINT_INTERVAL_MS) {
        _lastDebugPrint = millis();
        printStateInfo();
    }
    
    // Run the current state's logic
    _runCurrentStateLogic();
    
    // Update the clock in appropriate states (not during config, connecting, etc.)
    if (_currentState == STATE_RUNNING) {
        _clock.updateCurrentTime();
    }
}

void StateManager::transitionTo(ClockState newState) {
    if (_currentState == newState) {
        return; // No state change needed
    }
    
    // Validate state transition
    if (!_isValidTransition(_currentState, newState)) {
        Serial.print("Invalid state transition: ");
        Serial.print(_currentState);
        Serial.print(" -> ");
        Serial.println(newState);
        return;
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
    if (millis() - _configStartTime > CONFIG_TIMEOUT_MS) {
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
    if (millis() - _wifiConnectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
        // Reset NTP sync counter and return to running state
        _networkManager.resetNtpSyncCounter();
        transitionTo(STATE_RUNNING);
    }
}

void StateManager::_runSyncingTimeState() {
    // Attempt NTP sync using the RTC reference
    if (_networkManager.syncTimeWithRTC(_rtc)) {
        // After successful NTP sync, update clock to current time
        _clock.updateCurrentTime();
        transitionTo(STATE_RUNNING);
    }
    
    // Timeout after 30 seconds
    if (millis() - _ntpSyncStartTime > NTP_SYNC_TIMEOUT_MS) {
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
    static bool errorTimerStarted = false;
    
    if (!errorTimerStarted) {
        errorStartTime = millis();
        errorTimerStarted = true;
    }
    
    if (millis() - errorStartTime > ERROR_DISPLAY_TIMEOUT_MS) {
        errorTimerStarted = false; // Reset for next error
        transitionTo(STATE_INIT); // Try to recover
    }
}

bool StateManager::_isValidTransition(ClockState fromState, ClockState toState) const {
    // Define valid state transitions
    switch (fromState) {
        case STATE_INIT:
            return (toState == STATE_CONFIG || toState == STATE_CONNECTING_WIFI);
            
        case STATE_CONFIG:
            return (toState == STATE_CONNECTING_WIFI || toState == STATE_ERROR);
            
        case STATE_CONNECTING_WIFI:
            return (toState == STATE_SYNCING_TIME || toState == STATE_RUNNING || toState == STATE_ERROR);
            
        case STATE_SYNCING_TIME:
            return (toState == STATE_RUNNING || toState == STATE_ERROR);
            
        case STATE_RUNNING:
            return (toState == STATE_CONNECTING_WIFI || toState == STATE_SYNCING_TIME || toState == STATE_ERROR);
            
        case STATE_ERROR:
            return (toState == STATE_INIT);
            
        default:
            return false;
    }
}

 
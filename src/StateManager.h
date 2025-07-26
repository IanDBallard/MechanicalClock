#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <RTC.h>
#include "NetworkManager.h"
#include "LCDDisplay.h"
#include "Clock.h"

// State enumeration
enum ClockState {
    STATE_INIT = 0,
    STATE_CONFIG = 1,
    STATE_CONNECTING_WIFI = 2,
    STATE_SYNCING_TIME = 3,
    STATE_RUNNING = 4,
    STATE_ERROR = 5,
    STATE_POWER_SAVING = 6
};

class StateManager {
private:
    NetworkManager& _networkManager;
    LCDDisplay& _lcdDisplay;
    Clock& _clock;
    RTClock& _rtc;
    
    ClockState _currentState;
    String _lastError;
    unsigned long _lastStateChange;
    unsigned long _lastDebugPrint;
    
    // State-specific timing
    unsigned long _configStartTime;
    unsigned long _wifiConnectStartTime;
    unsigned long _ntpSyncStartTime;
    
    // State transition methods
    void _handleStateEntry(ClockState newState);
    void _handleStateExit(ClockState oldState);
    void _runCurrentStateLogic();
    
    // State-specific logic methods
    void _runInitState();
    void _runConfigState();
    void _runConnectingWiFiState();
    void _runSyncingTimeState();
    void _runRunningState();
    void _runErrorState();
    void _runPowerSavingState();

public:
    StateManager(NetworkManager& networkManager, LCDDisplay& lcdDisplay, 
                 Clock& clock, RTClock& rtc);
    
    // Main update method called from loop()
    void update();
    
    // State management
    void transitionTo(ClockState newState);
    ClockState getCurrentState() const;
    void setLastError(const String& error);
    String getLastError() const;
    
    // Debug and status
    void printStateInfo();
};

#endif // STATE_MANAGER_H 
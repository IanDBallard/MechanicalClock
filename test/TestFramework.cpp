#include "TestFramework.h"
#include <RTC.h>

// Global test registry instance
TestRegistry testRegistry;

// Mock implementations
int MockWiFi::mockWiFiStatus = WL_DISCONNECTED;
RTCTime MockRTC::mockCurrentTime = RTCTime(); 
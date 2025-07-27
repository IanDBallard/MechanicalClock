#include "TestFramework.h"
#include <RTC.h>

// Mock implementations
int MockWiFi::mockWiFiStatus = WL_DISCONNECTED;
RTCTime MockRTC::mockCurrentTime = RTCTime(); 
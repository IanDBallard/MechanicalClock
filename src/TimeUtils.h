#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <RTC.h> // For Month, DayOfWeek, RTCTime enums and struct

// Array of month names for display
extern const char* const MONTH_NAMES[];

// Array of abbreviated day of week names for display
extern const char* const DOW_ABBREV[];

// Note: Month2int and DayOfWeek2int are custom utility functions for converting
// RTC library enums to integers. They are defined in TimeUtils.cpp.

// Calculate if Daylight Saving Time (DST) is currently active for US rules
// This function takes an RTCTime object (assumed to be in UTC)
// and applies US DST rules to determine if DST is active.
// It will consider a default timezone for determining local dates when checking DST rules.
// For a fully robust solution, this might need more sophisticated timezone data.
bool calculateDST(RTCTime& time, int timeZoneOffsetHours); 

// UTC/Local time conversion functions
RTCTime convertUTCToLocal(time_t utcTime, int timeZoneOffsetHours, bool useDST);
time_t convertLocalToUTC(const RTCTime& localTime, int timeZoneOffsetHours, bool useDST);
time_t getCurrentUTC();

#endif // TIME_UTILS_H 
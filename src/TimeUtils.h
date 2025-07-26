#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <RTC.h> // For Month, DayOfWeek, RTCTime enums and struct

// Array of month names for display
extern const char* const MONTH_NAMES[];

// Array of abbreviated day of week names for display
extern const char* const DOW_ABBREV[];

// Note: Month2int and DayOfWeek2int are already provided by the RTC library
// No need to declare them here

// Calculate if Daylight Saving Time (DST) is currently active for US rules
// This function takes an RTCTime object (assumed to be in UTC)
// and applies US DST rules to determine if DST is active.
// It will consider a default timezone for determining local dates when checking DST rules.
// For a fully robust solution, this might need more sophisticated timezone data.
bool calculateDST(RTCTime& time, int timeZoneOffsetHours); 

#endif // TIME_UTILS_H 
#include "TimeUtils.h"
#include <Arduino.h> // For debugging Serial.println (can remove in final build)

// Array of month names for display (0-indexed)
const char* const MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Array of abbreviated day of week names for display (0-indexed, starting Sunday)
const char* const DOW_ABBREV[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Note: Month2int and DayOfWeek2int are already provided by the RTC library
// We don't need to implement them here to avoid duplicate definitions


// Calculate if Daylight Saving Time (DST) is currently active for US rules
// This function takes an RTCTime object (assumed to be in UTC)
// and applies US DST rules to determine if DST is active.
// timeZoneOffsetHours: The standard (non-DST) offset from UTC (e.g., -5 for EST)
bool calculateDST(RTCTime& utcTime, int timeZoneOffsetHours) {
    // Convert UTC time to local time before checking DST rules
    // Add timeZoneOffsetHours * 3600 seconds to UTC time
    time_t localEpoch = utcTime.getUnixTime() + (long)timeZoneOffsetHours * 3600L;
    RTCTime localTime(localEpoch);

    int year = localTime.getYear();
    int month = Month2int(localTime.getMonth()); // 1-12
    int day = localTime.getDayOfMonth();
    int hour = localTime.getHour(); // Local hour

    // Standard US DST Rules:
    // Starts: Second Sunday in March at 2:00 AM local time
    // Ends:   First Sunday in November at 2:00 AM local time

    // Rule: DST is not active in Jan, Feb, Dec.
    if (month < 3 || month > 11) {
        return false;
    }
    // Rule: DST is active for all of April through October.
    if (month > 3 && month < 11) {
        return true;
    }

    // Special handling for March (start of DST)
    if (month == 3) {
        // Find the date of the first Sunday in March
        int firstSundayDate = 0;
        for (int d_iter = 1; d_iter <= 7; ++d_iter) {
            // Create a temporary RTCTime for 2AM on this potential Sunday in local time
            RTCTime potentialSunday(d_iter, Month::MARCH, year, 2, 0, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_INACTIVE); 
            // Check if it's actually a Sunday
            if (DayOfWeek2int(potentialSunday.getDayOfWeek(), true) == 0) { // 0 for Sunday in RTC.h enum
                firstSundayDate = d_iter;
                break;
            }
        }
        int secondSundayDate = firstSundayDate + 7;
        
        // If current day is after the second Sunday
        if (day > secondSundayDate) {
            return true;
        }
        // If current day is the second Sunday
        if (day == secondSundayDate) {
            return hour >= 2; // DST starts at 2 AM
        }
        return false; // Before the second Sunday in March
    }

    // Special handling for November (end of DST)
    if (month == 11) {
        // Find the date of the first Sunday in November
        int firstSundayDate = 0;
        for (int d_iter = 1; d_iter <= 7; ++d_iter) {
            // Create a temporary RTCTime for 2AM on this potential Sunday in local time
            RTCTime potentialSunday(d_iter, Month::NOVEMBER, year, 2, 0, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_INACTIVE);
            // Check if it's actually a Sunday
            if (DayOfWeek2int(potentialSunday.getDayOfWeek(), true) == 0) { // 0 for Sunday in RTC.h enum
                firstSundayDate = d_iter;
                break;
            }
        }
        
        // If current day is before the first Sunday
        if (day < firstSundayDate) {
            return true; // Still in DST before the end date
        }
        // If current day is the first Sunday
        if (day == firstSundayDate) {
            return hour < 2; // DST ends at 2 AM
        }
        return false; // After the first Sunday in November
    }

    return false; // Should not be reached
} 
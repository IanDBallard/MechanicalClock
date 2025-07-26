#include "TestFramework.h"
#include "../src/TimeUtils.h"
#include <RTC.h>

TEST_SUITE(TimeUtilsTest);

void setupTimeUtilsTests() {
    // Test month names array
    ADD_TEST(testSuite_TimeUtilsTest, MonthNamesArray) {
        ASSERT_NOT_NULL(MONTH_NAMES);
        ASSERT_STRING_EQUAL("Jan", MONTH_NAMES[0]);
        ASSERT_STRING_EQUAL("Feb", MONTH_NAMES[1]);
        ASSERT_STRING_EQUAL("Mar", MONTH_NAMES[2]);
        ASSERT_STRING_EQUAL("Apr", MONTH_NAMES[3]);
        ASSERT_STRING_EQUAL("May", MONTH_NAMES[4]);
        ASSERT_STRING_EQUAL("Jun", MONTH_NAMES[5]);
        ASSERT_STRING_EQUAL("Jul", MONTH_NAMES[6]);
        ASSERT_STRING_EQUAL("Aug", MONTH_NAMES[7]);
        ASSERT_STRING_EQUAL("Sep", MONTH_NAMES[8]);
        ASSERT_STRING_EQUAL("Oct", MONTH_NAMES[9]);
        ASSERT_STRING_EQUAL("Nov", MONTH_NAMES[10]);
        ASSERT_STRING_EQUAL("Dec", MONTH_NAMES[11]);
    } END_TEST;

    // Test day of week abbreviations array
    ADD_TEST(testSuite_TimeUtilsTest, DayOfWeekAbbreviations) {
        ASSERT_NOT_NULL(DOW_ABBREV);
        ASSERT_STRING_EQUAL("Sun", DOW_ABBREV[0]);
        ASSERT_STRING_EQUAL("Mon", DOW_ABBREV[1]);
        ASSERT_STRING_EQUAL("Tue", DOW_ABBREV[2]);
        ASSERT_STRING_EQUAL("Wed", DOW_ABBREV[3]);
        ASSERT_STRING_EQUAL("Thu", DOW_ABBREV[4]);
        ASSERT_STRING_EQUAL("Fri", DOW_ABBREV[5]);
        ASSERT_STRING_EQUAL("Sat", DOW_ABBREV[6]);
    } END_TEST;

    // Test Month2int function (from RTC library)
    ADD_TEST(testSuite_TimeUtilsTest, Month2intFunction) {
        ASSERT_EQUAL(1, Month2int(Month::JANUARY));
        ASSERT_EQUAL(2, Month2int(Month::FEBRUARY));
        ASSERT_EQUAL(3, Month2int(Month::MARCH));
        ASSERT_EQUAL(4, Month2int(Month::APRIL));
        ASSERT_EQUAL(5, Month2int(Month::MAY));
        ASSERT_EQUAL(6, Month2int(Month::JUNE));
        ASSERT_EQUAL(7, Month2int(Month::JULY));
        ASSERT_EQUAL(8, Month2int(Month::AUGUST));
        ASSERT_EQUAL(9, Month2int(Month::SEPTEMBER));
        ASSERT_EQUAL(10, Month2int(Month::OCTOBER));
        ASSERT_EQUAL(11, Month2int(Month::NOVEMBER));
        ASSERT_EQUAL(12, Month2int(Month::DECEMBER));
    } END_TEST;

    // Test DayOfWeek2int function (from RTC library)
    ADD_TEST(testSuite_TimeUtilsTest, DayOfWeek2intFunction) {
        ASSERT_EQUAL(0, DayOfWeek2int(DayOfWeek::SUNDAY, true));
        ASSERT_EQUAL(1, DayOfWeek2int(DayOfWeek::MONDAY, true));
        ASSERT_EQUAL(2, DayOfWeek2int(DayOfWeek::TUESDAY, true));
        ASSERT_EQUAL(3, DayOfWeek2int(DayOfWeek::WEDNESDAY, true));
        ASSERT_EQUAL(4, DayOfWeek2int(DayOfWeek::THURSDAY, true));
        ASSERT_EQUAL(5, DayOfWeek2int(DayOfWeek::FRIDAY, true));
        ASSERT_EQUAL(6, DayOfWeek2int(DayOfWeek::SATURDAY, true));
    } END_TEST;

    // Test DST calculation - Winter (no DST)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationWinter) {
        // January 15, 2024 at 12:00 UTC (winter, no DST)
        RTCTime winterTime(15, Month::JANUARY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
        bool isDST = calculateDST(winterTime, -5); // EST timezone
        ASSERT_FALSE(isDST);
    } END_TEST;

    // Test DST calculation - Summer (DST active)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationSummer) {
        // July 15, 2024 at 12:00 UTC (summer, DST active)
        RTCTime summerTime(15, Month::JULY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
        bool isDST = calculateDST(summerTime, -5); // EST timezone
        ASSERT_TRUE(isDST);
    } END_TEST;

    // Test DST calculation - March transition (before DST starts)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationMarchBefore) {
        // March 9, 2024 at 12:00 UTC (before DST starts)
        RTCTime marchBefore(9, Month::MARCH, 2024, 12, 0, 0, DayOfWeek::SATURDAY, SaveLight::SAVING_TIME_INACTIVE);
        bool isDST = calculateDST(marchBefore, -5); // EST timezone
        ASSERT_FALSE(isDST);
    } END_TEST;

    // Test DST calculation - March transition (after DST starts)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationMarchAfter) {
        // March 11, 2024 at 12:00 UTC (after DST starts)
        RTCTime marchAfter(11, Month::MARCH, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
        bool isDST = calculateDST(marchAfter, -5); // EST timezone
        ASSERT_TRUE(isDST);
    } END_TEST;

    // Test DST calculation - November transition (before DST ends)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationNovemberBefore) {
        // November 2, 2024 at 12:00 UTC (before DST ends)
        RTCTime novemberBefore(2, Month::NOVEMBER, 2024, 12, 0, 0, DayOfWeek::SATURDAY, SaveLight::SAVING_TIME_ACTIVE);
        bool isDST = calculateDST(novemberBefore, -5); // EST timezone
        ASSERT_TRUE(isDST);
    } END_TEST;

    // Test DST calculation - November transition (after DST ends)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationNovemberAfter) {
        // November 4, 2024 at 12:00 UTC (after DST ends)
        RTCTime novemberAfter(4, Month::NOVEMBER, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
        bool isDST = calculateDST(novemberAfter, -5); // EST timezone
        ASSERT_FALSE(isDST);
    } END_TEST;

    // Test DST calculation with different timezones
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationDifferentTimezones) {
        // Same time, different timezone offsets
        RTCTime testTime(15, Month::JULY, 2024, 12, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
        
        // EST (-5)
        bool isDST_EST = calculateDST(testTime, -5);
        ASSERT_TRUE(isDST_EST);
        
        // CST (-6)
        bool isDST_CST = calculateDST(testTime, -6);
        ASSERT_TRUE(isDST_CST);
        
        // MST (-7)
        bool isDST_MST = calculateDST(testTime, -7);
        ASSERT_TRUE(isDST_MST);
        
        // PST (-8)
        bool isDST_PST = calculateDST(testTime, -8);
        ASSERT_TRUE(isDST_PST);
    } END_TEST;

    // Test edge cases - DST start time (2 AM)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationEdgeCaseStart) {
        // March 10, 2024 at 1:59 AM local time (just before DST starts)
        RTCTime beforeDST(10, Month::MARCH, 2024, 6, 59, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_INACTIVE); // 1:59 AM EST
        bool isDST_before = calculateDST(beforeDST, -5);
        ASSERT_FALSE(isDST_before);
        
        // March 10, 2024 at 2:00 AM local time (DST starts)
        RTCTime atDST(10, Month::MARCH, 2024, 7, 0, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_ACTIVE); // 2:00 AM EDT
        bool isDST_at = calculateDST(atDST, -5);
        ASSERT_TRUE(isDST_at);
    } END_TEST;

    // Test edge cases - DST end time (2 AM)
    ADD_TEST(testSuite_TimeUtilsTest, DSTCalculationEdgeCaseEnd) {
        // November 3, 2024 at 1:59 AM local time (just before DST ends)
        RTCTime beforeEnd(3, Month::NOVEMBER, 2024, 6, 59, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_ACTIVE); // 1:59 AM EDT
        bool isDST_before = calculateDST(beforeEnd, -5);
        ASSERT_TRUE(isDST_before);
        
        // November 3, 2024 at 2:00 AM local time (DST ends)
        RTCTime atEnd(3, Month::NOVEMBER, 2024, 7, 0, 0, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_INACTIVE); // 2:00 AM EST
        bool isDST_at = calculateDST(atEnd, -5);
        ASSERT_FALSE(isDST_at);
    } END_TEST;

    testRegistry.addSuite(testSuite_TimeUtilsTest);
} 
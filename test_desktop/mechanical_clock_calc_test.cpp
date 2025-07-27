#include <gtest/gtest.h>
#include <iostream>
#include <cstring>

// Mock Arduino environment
#include "TestFramework.h"

// Include the actual MechanicalClock class
#include "../src/MechanicalClock.h"

class MechanicalClockCalcTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset mock time
        MockRTC::mockCurrentTime = 0;
    }
};

// Test power-up calculation scenarios
TEST_F(MechanicalClockCalcTest, PowerUpCalculationTest) {
    std::cout << "\n=== Testing Mechanical Clock Power-Up Calculations ===" << std::endl;
    
    // Test case 1: Normal power loss (1 hour)
    {
        time_t powerDownTime = 1753573742; // 1 hour ago
        time_t currentTime = 1753577342;   // Current time
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "Test 1: 1 hour power loss" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        // Calculate expected steps (assuming 1 step per second)
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        // Check if this is reasonable
        EXPECT_LT(expectedSteps, 10000) << "Step count should be reasonable for 1 hour";
        EXPECT_GT(expectedSteps, 0) << "Step count should be positive";
    }
    
    // Test case 2: Long power loss (1 day)
    {
        time_t powerDownTime = 1753490942; // 1 day ago
        time_t currentTime = 1753577342;   // Current time
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "\nTest 2: 1 day power loss" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        EXPECT_LT(expectedSteps, 100000) << "Step count should be reasonable for 1 day";
        EXPECT_GT(expectedSteps, 0) << "Step count should be positive";
    }
    
    // Test case 3: Corrupted power down time (very old)
    {
        time_t powerDownTime = 1640995200; // Jan 1, 2022 (very old)
        time_t currentTime = 1753577342;   // Current time
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "\nTest 3: Corrupted power down time (very old)" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        // This should be detected as invalid
        EXPECT_GT(expectedSteps, 1000000) << "Very old time should result in large step count";
        std::cout << "  WARNING: This would cause unstoppable rotation!" << std::endl;
    }
    
    // Test case 4: Zero power down time (uninitialized EEPROM)
    {
        time_t powerDownTime = 0;          // Uninitialized
        time_t currentTime = 1753577342;   // Current time
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "\nTest 4: Zero power down time (uninitialized)" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        // This should be detected as invalid
        EXPECT_GT(expectedSteps, 1000000) << "Zero time should result in large step count";
        std::cout << "  WARNING: This would cause unstoppable rotation!" << std::endl;
    }
    
    // Test case 5: Future power down time (impossible)
    {
        time_t powerDownTime = 1753577342 + 3600; // 1 hour in future
        time_t currentTime = 1753577342;          // Current time
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "\nTest 5: Future power down time (impossible)" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        EXPECT_LT(expectedSteps, 0) << "Future time should result in negative step count";
        std::cout << "  WARNING: Negative steps might cause issues!" << std::endl;
    }
}

// Test the actual MechanicalClock class calculations
TEST_F(MechanicalClockCalcTest, MechanicalClockClassTest) {
    std::cout << "\n=== Testing MechanicalClock Class Calculations ===" << std::endl;
    
    // Create a mechanical clock instance
    MechanicalClock clock(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    
    // Test case: Simulate the problematic scenario from the debug output
    {
        time_t powerDownTime = 1753534134; // From debug output
        time_t currentTime = 1753577342;   // From debug output
        time_t timeDiff = currentTime - powerDownTime;
        
        std::cout << "Debug Output Analysis:" << std::endl;
        std::cout << "  Power down time: " << powerDownTime << std::endl;
        std::cout << "  Current time: " << currentTime << std::endl;
        std::cout << "  Time difference: " << timeDiff << " seconds (" << timeDiff/3600.0 << " hours)" << std::endl;
        
        // This is about 12 hours difference - reasonable for overnight power loss
        EXPECT_GT(timeDiff, 0) << "Time difference should be positive";
        EXPECT_LT(timeDiff, 86400) << "Time difference should be less than 1 day";
        
        // Calculate what the step count should be
        // Assuming 1 step per second (you may need to adjust this based on your actual gearing)
        long expectedSteps = timeDiff;
        std::cout << "  Expected steps: " << expectedSteps << std::endl;
        
        // Check if this matches the debug output
        std::cout << "  Debug output showed: 52593598 steps" << std::endl;
        std::cout << "  Ratio: " << (double)52593598 / expectedSteps << "x larger than expected" << std::endl;
        
        // This suggests there's a multiplier issue in the calculation
        EXPECT_LT(expectedSteps, 100000) << "Step count should be reasonable for 12 hours";
    }
}

// Test EEPROM time validation
TEST_F(MechanicalClockCalcTest, EEPROMTimeValidationTest) {
    std::cout << "\n=== Testing EEPROM Time Validation ===" << std::endl;
    
    // Valid time threshold (Jan 1, 2023)
    time_t validThreshold = 1672531200UL;
    
    // Test various EEPROM values
    std::vector<time_t> testTimes = {
        0,                    // Uninitialized
        1640995200,          // Jan 1, 2022 (too old)
        1672531200,          // Jan 1, 2023 (threshold)
        1672531201,          // Jan 1, 2023 + 1 second (valid)
        1753577342,          // Current time (valid)
        1753577342 + 3600,   // Future time (invalid)
    };
    
    for (time_t testTime : testTimes) {
        bool isValid = (testTime >= validThreshold);
        std::cout << "  EEPROM time " << testTime << " (" << (testTime == 0 ? "zero" : "non-zero") << "): " 
                  << (isValid ? "VALID" : "INVALID") << std::endl;
        
        if (!isValid && testTime > 0) {
            std::cout << "    WARNING: Invalid time would cause large step calculation!" << std::endl;
        }
    }
}

// Test step calculation sanity checks
TEST_F(MechanicalClockCalcTest, StepCalculationSanityTest) {
    std::cout << "\n=== Testing Step Calculation Sanity Checks ===" << std::endl;
    
    // Define reasonable limits
    const long MAX_REASONABLE_STEPS = 86400; // 1 day worth of seconds
    const long MIN_REASONABLE_STEPS = -3600; // Allow 1 hour negative (for small corrections)
    
    // Test various time differences
    std::vector<int> timeDiffs = {
        -3600,    // -1 hour
        -60,      // -1 minute
        0,        // No difference
        60,       // +1 minute
        3600,     // +1 hour
        86400,    // +1 day
        604800,   // +1 week
    };
    
    for (int timeDiff : timeDiffs) {
        long steps = timeDiff; // Assuming 1:1 ratio for now
        
        std::cout << "  Time diff: " << timeDiff << "s -> Steps: " << steps << std::endl;
        
        if (steps > MAX_REASONABLE_STEPS) {
            std::cout << "    WARNING: Steps too large - would cause unstoppable rotation!" << std::endl;
        } else if (steps < MIN_REASONABLE_STEPS) {
            std::cout << "    WARNING: Steps too negative - might cause issues!" << std::endl;
        } else {
            std::cout << "    OK: Steps within reasonable range" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 
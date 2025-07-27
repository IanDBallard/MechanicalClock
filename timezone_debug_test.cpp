#include <iostream>
#include <ctime>
#include <vector> // Added missing include for std::vector

// Simple test to debug timezone issues
int main() {
    std::cout << "=== Timezone Debug Test ===" << std::endl;
    
    // Current Unix timestamp (approximate)
    time_t currentTime = 1753578000; // Around the time from your debug output
    
    std::cout << "Current Unix timestamp: " << currentTime << std::endl;
    
    // Test different timezone offsets
    std::vector<int> timezoneOffsets = {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5};
    
    for (int offset : timezoneOffsets) {
        time_t localTime = currentTime + (offset * 3600);
        
        // Convert to readable time
        struct tm* timeinfo = gmtime(&localTime);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        std::cout << "UTC" << (offset >= 0 ? "+" : "") << offset << ": " << timeStr << std::endl;
    }
    
    std::cout << "\n=== Timezone Offset Guide ===" << std::endl;
    std::cout << "Common US Timezones:" << std::endl;
    std::cout << "  PST (Pacific): -8" << std::endl;
    std::cout << "  PDT (Pacific Daylight): -7" << std::endl;
    std::cout << "  MST (Mountain): -7" << std::endl;
    std::cout << "  MDT (Mountain Daylight): -6" << std::endl;
    std::cout << "  CST (Central): -6" << std::endl;
    std::cout << "  CDT (Central Daylight): -5" << std::endl;
    std::cout << "  EST (Eastern): -5" << std::endl;
    std::cout << "  EDT (Eastern Daylight): -4" << std::endl;
    
    std::cout << "\nIf your clock shows 4 hours ahead of local time:" << std::endl;
    std::cout << "  - Current setting: -5 (EST)" << std::endl;
    std::cout << "  - If you're in PST (-8), you need: -8" << std::endl;
    std::cout << "  - If you're in MST (-7), you need: -7" << std::endl;
    std::cout << "  - If you're in CST (-6), you need: -6" << std::endl;
    std::cout << "  - If you're in EDT (-4), you need: -4" << std::endl;
    
    return 0;
} 
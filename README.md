# Mechanical Clock with Onboard RTC

A sophisticated Arduino-based mechanical clock system featuring a stepper motor-driven clock mechanism with real-time clock synchronization, WiFi connectivity, and smart LCD display management.

## 🕐 Overview

This project implements a fully functional mechanical clock using:
- **Arduino Uno R4 WiFi** as the main controller
- **Stepper motor** for precise hand movement
- **Onboard RTC** for timekeeping
- **WiFi connectivity** for NTP time synchronization
- **16x2 LCD display** with smart buffer management
- **Status indicators** for WiFi and sync status

## 🏗️ Architecture

The system uses a modular, object-oriented design with the following key components:

- **`MechanicalClock`** - Stepper motor control and hand positioning
- **`DigitalClock`** - Alternative digital-only implementation
- **`LCDDisplay`** - Smart buffer system with constrained areas
- **`NetworkManager`** - WiFi and NTP synchronization
- **`StateManager`** - Finite state machine for system orchestration
- **`TimeUtils`** - Time conversion and formatting utilities

## 📁 Project Structure

```
MechanicalClockwithOnboardRTC/
├── src/                    # Main source code
│   ├── main.cpp           # Application entry point
│   ├── MechanicalClock.cpp # Stepper motor control
│   ├── LCDDisplay.cpp     # Smart LCD buffer system
│   ├── NetworkManager.cpp # WiFi and NTP management
│   ├── StateManager.cpp   # State machine implementation
│   └── TimeUtils.cpp      # Time utilities
├── test/                  # Comprehensive test suite
├── platformio.ini         # PlatformIO configuration
└── Documentation/         # Detailed documentation
```

## 🚀 Quick Start

### Prerequisites
- Arduino Uno R4 WiFi
- 16x2 I2C LCD Display
- Stepper motor with driver
- PlatformIO IDE or Arduino IDE

### Installation
1. Clone this repository
2. Open in PlatformIO IDE
3. Configure WiFi credentials in `Constants.h`
4. Build and upload to your Arduino Uno R4 WiFi

### Configuration
Edit `src/Constants.h` to set:
- WiFi credentials
- NTP server settings
- Timezone configuration
- Hardware pin assignments

## 📚 Documentation

This project includes comprehensive documentation:

- **[DOCUMENTATION.md](DOCUMENTATION.md)** - Complete technical reference
- **[FiniteStateMachine.md](FiniteStateMachine.md)** - State machine design
- **[DesignDecisions.md](DesignDecisions.md)** - Architecture decisions
- **[TEST_ENVIRONMENTS.md](TEST_ENVIRONMENTS.md)** - Testing infrastructure
- **[last-Chat-Summary.md](last-Chat-Summary.md)** - Development history

## 🧪 Testing

The project includes a comprehensive test suite:

```bash
# Run all tests
pio run -e uno_r4_wifi_test

# Run specific test environments
pio run -e uno_r4_wifi_clock_advance_test

# Use test runner scripts
./run_tests.sh                    # Main test runner
./run_test_monitor.sh             # Monitor test output
./run_clock_advance_test.sh       # Clock advance testing
```

## 🔧 Key Features

### Smart LCD Buffer System
- **Constrained areas** - Each display method writes only to its designated area
- **Flicker reduction** - Only changed characters are written to the LCD
- **Status icon preservation** - WiFi and sync indicators remain visible during updates

### Network Synchronization
- **Automatic WiFi connection** with credential storage in EEPROM
- **NTP time synchronization** with configurable intervals
- **Graceful reconnection** handling

### State Management
- **Robust state machine** with validation
- **Power recovery** from EEPROM-stored state
- **Error handling** with automatic recovery

### Mechanical Control
- **Precise stepper motor control** with microstepping
- **Hand position tracking** and synchronization
- **Power-efficient operation**

## 🛠️ Build Environments

The project supports multiple build configurations:

- **`uno_r4_wifi`** - Main application
- **`uno_r4_wifi_test`** - Test environment
- **`uno_r4_wifi_clock_advance_test`** - Clock advance testing

## 📊 Memory Usage

- **RAM**: ~29.7% (9,732 bytes of 32,768 bytes)
- **Flash**: ~37.2% (97,568 bytes of 262,144 bytes)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This project is open source. See the license file for details.

## 🆘 Support

For issues and questions:
1. Check the documentation files
2. Review the test suite for examples
3. Open an issue on the repository

---

**Version**: 2.1.15  
**Last Updated**: December 2024  
**Platform**: Arduino Uno R4 WiFi (Renesas RA4M1)
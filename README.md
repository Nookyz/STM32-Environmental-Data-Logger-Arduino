# STM32 Environmental Data Logger

A modular environmental monitoring system based on **STM32F401CCU**, developed in **C++** using the **Arduino framework** and **PlatformIO**.

The project collects environmental data from AHT20 and BMP280 sensors, processes and validates measurements, provides a UART command-line interface, controls an LED and button, and stores calibration data in external W25Q128 SPI Flash memory.

This project was developed as a practical Embedded C/C++ learning project with a focus on modular firmware architecture, hardware interfaces, sensor data processing, non-volatile storage, and reusable C++ components.

---

## Features

* STM32F401CCU microcontroller
* Embedded C++
* Arduino framework
* PlatformIO
* GPIO
* Button input with internal pull-up
* Button debouncing
* Single / double / triple click detection
* LED control and blinking modes
* UART command-line interface
* I2C communication
* SPI communication
* AHT20 temperature and humidity sensor
* BMP280 temperature and pressure sensor
* W25Q128 external SPI Flash
* Persistent calibration storage
* CRC-based calibration data validation
* Flash read-back verification
* Moving-average filtering
* Sensor data validation
* Fixed-point sensor representation
* Timer-based periodic sensor monitoring
* Modular driver architecture
* Separation of application, communication, drivers, sensors, storage, and utilities
* Unit-test structure for project components

---

# Hardware

## Main Components

| Component    | Purpose                                          |
| ------------ | ------------------------------------------------ |
| STM32F401CCU | Main microcontroller                             |
| AHT20        | Temperature and humidity measurement             |
| BMP280       | Temperature and atmospheric pressure measurement |
| W25Q128      | External non-volatile SPI Flash                  |
| Push Button  | User input and click detection                   |
| LED          | Status and operating mode indication             |
| CP2102       | USB-to-UART communication                        |
| ST-Link V2   | Programming and debugging                        |

---

# Pinout

## STM32F401CCU

The following pins are used by the current firmware:

| STM32 Pin | Connected Device | Function        |
| --------- | ---------------- | --------------- |
| `PB0`     | Push Button      | GPIO input      |
| `PB1`     | LED              | GPIO output     |
| `PB6`     | AHT20 / BMP280   | I2C SCL         |
| `PB7`     | AHT20 / BMP280   | I2C SDA         |
| `PA4`     | W25Q128          | SPI Chip Select |
| `3.3V`    | Modules          | Power           |
| `GND`     | Modules          | Common ground   |

---

## Button

The push button is connected between `PB0` and `GND`.

The firmware uses the STM32 internal pull-up:

```cpp
pinMode(PB0, INPUT_PULLUP);
```

Therefore:

```text
Button released -> HIGH
Button pressed  -> LOW
```

The button driver also performs software debouncing.

Current debounce time:

```cpp
50 ms
```

---

## LED

The LED is connected to `PB1` through a current-limiting resistor.

```text
PB1 ── Resistor ── LED ── GND
```

The LED is controlled by the `Led` driver.

Available modes:

* `OFF`
* `ON`
* `SLOW_BLINK`
* `FAST_BLINK`

Current blink intervals:

```text
Slow blink: 500 ms
Fast blink: 100 ms
```

---

# I2C Bus

The AHT20 and BMP280 share the same I2C bus.

```text
STM32
  │
  ├── PB6 ── SCL ── AHT20
  │                 BMP280
  │
  └── PB7 ── SDA ── AHT20
                    BMP280
```

Current configuration:

```cpp
#define PIN_SCL PB6
#define PIN_SDA PB7
```

Both sensors can operate on the same bus because they use different I2C addresses.

---

# W25Q128 SPI Flash

The W25Q128 is used as external non-volatile storage.

The current firmware uses:

```cpp
constexpr uint8_t PIN_FLASH_CS = PA4;
```

Therefore:

```text
STM32 PA4 -> W25Q128 CS
```

The SPI peripheral provides the remaining SPI signals.

The Flash is currently used for persistent sensor calibration data.

> The exact SPI SCK/MISO/MOSI mapping depends on the SPI peripheral/board configuration used by the current PlatformIO/Arduino STM32 variant and is intentionally not listed here without an explicit pin definition in the project source.

---

# UART Communication

The project provides a text-based command interface through `Serial2`.

Configuration:

```cpp
Serial2.begin(115200);
```

The CP2102 USB-UART adapter is used to connect the STM32 to a PC.

Typical connection:

```text
STM32 TX -> CP2102 RX
STM32 RX -> CP2102 TX
STM32 GND -> CP2102 GND
```

The UART terminal operates at:

```text
115200 baud
```

Example PlatformIO command:

```powershell
pio device monitor -p COM3 -b 115200
```

The COM port can be different depending on the Windows device configuration.

---

# ST-Link V2

ST-Link V2 is used for firmware programming and debugging through SWD.

Typical connection:

```text
ST-Link V2       STM32F401CCU
--------------------------------
3.3V       ->    3.3V
GND        ->    GND
SWCLK      ->    SWCLK
SWDIO      ->    SWDIO
```

PlatformIO uses:

```ini
upload_protocol = stlink
```

ST-Link and CP2102 have different purposes:

```text
ST-Link -> programming / debugging
CP2102  -> UART communication
```

---

# Software Architecture

The firmware is divided into several modules.

```text
STM32-Environmental-Data-Logger
│
├── include/
│   ├── app/
│   ├── communication/
│   ├── drivers/
│   ├── sensors/
│   └── utils/
│
├── src/
│   ├── app/
│   ├── communication/
│   ├── drivers/
│   ├── sensors/
│   ├── storage/
│   └── utils/
│
├── test/
│   └── test_led/
│
├── platformio.ini
└── .gitignore
```

---

# Project Structure

```text
include/
│
├── app/
│   └── app.hpp
│
├── communication/
│   └── Command.hpp
│
├── drivers/
│   ├── Button.hpp
│   ├── Led.hpp
│   └── W25Q128.hpp
│
├── sensors/
│   ├── AHT20.hpp
│   └── BMP280.hpp
│
└── utils/
    ├── ClickDetector.hpp
    └── Timer.hpp


src/
│
├── main.cpp
│
├── app/
│   └── app.cpp
│
├── communication/
│   └── Command.cpp
│
├── drivers/
│   ├── Button.cpp
│   ├── Led.cpp
│   └── W25Q128.cpp
│
├── sensors/
│   ├── AHT20.cpp
│   └── BMP280.cpp
│
├── storage/
│   └── CalibrationStorage.hpp
│
└── utils/
    ├── Calibration.h
    ├── ClickDetector.cpp
    ├── ConsoleTable.h
    ├── DataValidator.h
    ├── MovingAverage.h
    └── Timer.cpp


test/
│
└── test_led/
    └── test_main.cpp
```

---

# Application Layer

## App

`App` is the main application coordinator.

It connects the different components of the system:

* Button
* LED
* ClickDetector
* AHT20
* BMP280
* W25Q128
* CalibrationStorage
* Command

The application is responsible for initialization and the main update cycle.

Conceptually:

```text
                ┌──────────────┐
                │     App      │
                └──────┬───────┘
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
    Button            LED            Command
                                        │
                         ┌──────────────┼──────────────┐
                         ▼              ▼              ▼
                       AHT20          BMP280        Flash
                         │              │              │
                         └──────────────┴──────────────┘
```

---

# Communication Layer

## Command

The `Command` class implements the UART command-line interface.

Responsibilities include:

* receiving commands
* parsing commands
* controlling the LED
* reading sensors
* displaying sensor data
* enabling/disabling WATCH mode
* performing calibration
* saving calibration
* displaying help

The command parser operates using a fixed-size buffer.

This avoids dynamic memory allocation for command input.

---

# Driver Layer

## Button

Responsible for:

* button initialization
* reading GPIO state
* debouncing
* detecting button press events

---

## Led

Responsible for:

* LED initialization
* ON/OFF state
* slow blinking
* fast blinking
* non-blocking LED updates

---

## W25Q128

Responsible for communication with the external SPI Flash.

The driver provides the low-level operations required by the storage layer.

The application does not need to know the low-level SPI protocol details.

---

# Sensor Layer

## AHT20

Provides:

* temperature
* relative humidity

Temperature and humidity are processed using integer/fixed-point representations.

Example:

```text
2537 -> 25.37 °C
5000 -> 50.00 %
```

---

## BMP280

Provides:

* temperature
* atmospheric pressure

Pressure is represented in Pascals.

Example:

```text
100812 Pa
```

equals:

```text
1008.12 hPa
```

---

# Storage Layer

## CalibrationStorage

`CalibrationStorage` is responsible for storing and loading calibration information from W25Q128.

The stored structure contains:

```cpp
struct CalibrationData
{
    uint32_t magic;
    int32_t ahtTemperatureOffset;
    int32_t bmpTemperatureOffset;
    int32_t pressureOffset;
    uint16_t version;
    uint16_t crc;
};
```

The structure is currently expected to occupy:

```text
20 bytes
```

---

# Calibration Storage Integrity

The stored calibration data contains several mechanisms for detecting invalid data.

## Magic Number

A magic value identifies valid application data:

```cpp
0xCA11B001
```

---

## Version

A version field allows the data format to evolve without blindly accepting incompatible data.

Current version:

```text
1
```

---

## CRC

A CRC-16 checksum is calculated over the calibration data.

When loading:

```text
Flash data
    │
    ▼
Check magic
    │
    ▼
Check version
    │
    ▼
Calculate CRC
    │
    ▼
Compare CRC
    │
    ▼
Valid calibration
```

If validation fails, the stored data is rejected.

---

## Read-back Verification

After writing calibration data, the firmware reads the data back from Flash and compares it with the original structure.

This verifies that the write operation completed correctly.

---

# Calibration

The `CAL` command performs automatic calibration using the current sensor readings.

Basic workflow:

```text
CAL
 │
 ├── Read AHT20
 │
 ├── Read BMP280
 │
 ├── Validate readings
 │
 ├── Calculate calibration offsets
 │
 ├── Save to W25Q128
 │
 └── Verify saved data
```

The calibration is then persistent across:

* MCU reset
* power cycle
* firmware reprogramming

unless the corresponding Flash sector is erased or overwritten.

---

## Important Calibration Note

The current implementation performs a **zero/baseline calibration** based on the current sensor readings.

This is different from laboratory/absolute calibration.

For example, if the sensor currently measures:

```text
25.37 °C
```

the calibration offset can be used to treat the current measurement as the selected baseline.

True absolute sensor calibration requires a known external reference.

For example:

```text
Reference temperature: 25.00 °C
Sensor temperature:     25.37 °C

Offset = 25.00 - 25.37
```

The same principle applies to pressure and humidity when a reliable reference is available.

---

# Sensor Data Processing

The sensor data follows this processing pipeline:

```text
              Sensor
                 │
                 ▼
             Raw value
                 │
                 ▼
          Data validation
                 │
                 ▼
        Moving average filter
                 │
                 ▼
           Calibration
                 │
                 ▼
          Final value
```

This separation makes each processing stage independently testable and reusable.

---

# Moving Average

The project uses a templated moving-average filter:

```cpp
MovingAverage<int32_t, 5>
```

for:

* AHT20 temperature
* BMP280 temperature
* pressure

The filter uses multiple recent samples to reduce short-term measurement noise.

Current window size:

```text
5 samples
```

---

# Data Validation

Sensor readings are checked for valid ranges and implausible changes.

Current configured ranges include:

### Temperature

```text
-40.00 °C ... +85.00 °C
```

### Humidity

```text
0.00 % ... 100.00 %
```

### Pressure

```text
30000 Pa ... 110000 Pa
```

The validator also provides a mechanism for detecting implausible jumps between consecutive measurements.

---

# Timer

The project contains a reusable `Timer` utility.

It is used for time-based operations without blocking the main application with long delays.

One example is the `WATCH` command.

Current sensor monitoring interval:

```text
2000 ms
```

---

# Click Detection

The `ClickDetector` groups individual button presses into click sequences.

Current behavior:

| Clicks | Action            |
| -----: | ----------------- |
|      1 | Toggle LED ON/OFF |
|      2 | Slow blink        |
|     3+ | Fast blink        |

Click grouping timeout:

```text
500 ms
```

Button debounce:

```text
50 ms
```

---

# UART Commands

The firmware currently supports the following commands:

| Command  | Description                            |
| -------- | -------------------------------------- |
| `ON`     | Turn LED on                            |
| `OFF`    | Turn LED off                           |
| `SLOW`   | Enable slow blinking                   |
| `FAST`   | Enable fast blinking                   |
| `STATUS` | Show current LED mode                  |
| `READ`   | Read and display sensor data           |
| `WATCH`  | Enable/disable periodic sensor output  |
| `CAL`    | Calibrate sensors and save calibration |
| `SAVE`   | Save current calibration to Flash      |
| `HELP`   | Show available commands                |

---

# Command Examples

### Turn LED on

```text
ON
```

### Turn LED off

```text
OFF
```

### Slow blinking

```text
SLOW
```

### Fast blinking

```text
FAST
```

### Show LED status

```text
STATUS
```

### Read sensor values

```text
READ
```

### Enable/disable monitoring

```text
WATCH
```

### Calibrate sensors

```text
CAL
```

### Save calibration

```text
SAVE
```

### Show help

```text
HELP
```

---

# WATCH Mode

`WATCH` enables periodic sensor reporting.

Current interval:

```cpp
WATCH_INTERVAL_MS = 2000;
```

Therefore sensor information is displayed approximately every two seconds while WATCH mode is enabled.

The timer-based approach allows the rest of the application to continue running.

---

# Example Data Flow

A typical sensor measurement follows this sequence:

```text
AHT20
  │
  ▼
Raw temperature / humidity
  │
  ▼
Validation
  │
  ▼
MovingAverage
  │
  ▼
Calibration
  │
  ▼
ConsoleTable
  │
  ▼
UART / CP2102
  │
  ▼
PC terminal
```

The same general approach is used for BMP280 temperature and pressure.

---

# Main Application Loop

The application periodically updates the system.

Conceptually:

```cpp
void loop()
{
    app.update();
}
```

Inside the update cycle:

```text
Button update
      │
      ▼
Button event
      │
      ▼
ClickDetector
      │
      ▼
LED mode update

and

Command update
      │
      ▼
UART command processing
      │
      ▼
Sensor operations
```

The architecture avoids putting all application logic directly inside `main.cpp`.

---

# PlatformIO

The project uses PlatformIO as the build and upload system.

The target environment is:

```text
genericSTM32F401CC
```

Framework:

```text
Arduino
```

Platform:

```text
ststm32
```

Programming protocol:

```text
ST-Link
```

Serial monitor speed:

```text
115200
```

The main project configuration is stored in:

```text
platformio.ini
```

---

# Testing

The project contains a test directory:

```text
test/
└── test_led/
    └── test_main.cpp
```

This provides a foundation for automated unit testing.

The architecture separates reusable logic into classes, making it possible to test individual components independently from the physical STM32 hardware.

Potential candidates for unit testing include:

* `ClickDetector`
* `Led`
* command parsing logic

---

# Embedded C++ Concepts Demonstrated

The project intentionally uses several Embedded C++ concepts.

### Classes

Hardware and application functionality are encapsulated into classes:

```text
App
Command
Button
Led
W25Q128
AHT20
BMP280
CalibrationStorage
ClickDetector
Timer
```

### References

Hardware dependencies are passed to classes by reference.

For example:

```cpp
Command(
    Led &led,
    AHT20 &aht20,
    BMP280 &bmp280,
    CalibrationStorage &calibrationStorage);
```

This avoids unnecessary object copies and makes dependencies explicit.

### Templates

Generic utilities are implemented using templates.

For example:

```cpp
MovingAverage<int32_t, 5>
```

and:

```cpp
Calibration<int32_t>
```

This allows reusable components to work with different data types.

### `constexpr`

Compile-time constants are used for configuration:

```cpp
constexpr uint16_t CLICK_TIMEOUT = 500;
constexpr uint16_t DEBOUNCE_TIME = 50;
```

### `volatile`

Hardware/event-related state can use `volatile` where values may be changed asynchronously.

### Header / Source Separation

The project separates declarations and implementations:

```text
.hpp / .h
    │
    └── declarations

.cpp
    │
    └── implementations
```

This makes the project easier to maintain and scale.

---

# Interfaces Used

The project currently demonstrates the following embedded interfaces:

| Interface | Usage                         |
| --------- | ----------------------------- |
| GPIO      | Button and LED                |
| UART      | PC command interface          |
| I2C       | AHT20 and BMP280              |
| SPI       | W25Q128 Flash                 |
| SWD       | ST-Link programming/debugging |

---

# Memory and Persistence

The STM32F401CCU contains internal Flash memory used for the firmware itself.

The project additionally uses external:

```text
W25Q128 SPI Flash
```

for application data.

Currently stored:

```text
Sensor calibration
```

This separation allows application configuration to survive firmware updates, provided the external Flash is not erased.

---

# Repository Files

The main repository contains:

```text
.gitignore
platformio.ini

include/
src/
test/
```

The PlatformIO-generated `.pio` directory is build output and should not be committed to Git.

---

# Files That Should Not Be Committed

The following generated directory should remain ignored:

```text
.pio/
```

It contains:

* compiled object files
* firmware binaries
* linker output
* generated build files
* downloaded build dependencies

These files are generated automatically by PlatformIO.

---

# Build

Open the project in Visual Studio Code with PlatformIO installed.

Build:

```powershell
pio run
```

Upload using ST-Link:

```powershell
pio run -t upload
```

Open the UART monitor:

```powershell
pio device monitor -p COM3 -b 115200
```

Replace `COM3` with the actual CP2102 COM port.

---

# Development Workflow

Typical development workflow:

```text
Edit source code
      │
      ▼
PlatformIO build
      │
      ▼
ST-Link upload
      │
      ▼
STM32 firmware execution
      │
      ▼
CP2102 UART
      │
      ▼
PC terminal
      │
      ▼
Test / debug
```

---

# Current Implementation Status

## Hardware

* [x] STM32F401CCU
* [x] ST-Link V2
* [x] CP2102
* [x] LED
* [x] Push button
* [x] AHT20
* [x] BMP280
* [x] W25Q128

## Firmware

* [x] C++ application structure
* [x] Arduino framework
* [x] PlatformIO
* [x] GPIO
* [x] Button driver
* [x] LED driver
* [x] Button debouncing
* [x] Click detection
* [x] LED modes
* [x] UART command parser
* [x] I2C sensor communication
* [x] SPI Flash communication
* [x] Sensor data processing
* [x] Moving average
* [x] Data validation
* [x] Calibration
* [x] Persistent calibration
* [x] CRC validation
* [x] Flash read-back verification
* [x] Timer-based WATCH mode
* [x] Wokwi project configuration
* [x] Initial unit-test structure

---

# Learning Objectives

This project was created to develop practical skills in:

* Embedded C/C++
* STM32 microcontrollers
* ARM Cortex-M development
* Hardware abstraction
* GPIO
* UART
* I2C
* SPI
* Interrupt-oriented programming
* Sensor interfacing
* Data filtering
* Data validation
* Non-volatile memory
* CRC
* Persistent configuration
* Modular firmware architecture
* Object-oriented programming in embedded systems
* PlatformIO
* ST-Link debugging and programming
* Embedded software testing

---

# Summary

**STM32 Environmental Data Logger** is a modular Embedded C++ project combining real hardware interfaces with reusable software components.

The firmware demonstrates a complete data path:

```text
       ┌──────────────┐
       │    AHT20     │
       │ Temp/Humidity│
       └──────┬───────┘
              │
              │ I2C
              │
       ┌──────▼───────┐
       │     STM32    │
       │   F401CCU    │
       └──────┬───────┘
              │
      ┌───────┼──────────────┐
      │       │              │
      ▼       ▼              ▼
   BMP280   W25Q128       UART/CP2102
   I2C      SPI Flash          │
      │       │                ▼
      │       │             PC Terminal
      │       │
      │       └── Calibration
      │
      └── Sensor data
              │
              ▼
        Validation
              │
              ▼
        Moving Average
              │
              ▼
         Calibration
              │
              ▼
        Processed Data
```

The project is intended as a practical portfolio project demonstrating the transition from general software development to Embedded C/C++ development on STM32.

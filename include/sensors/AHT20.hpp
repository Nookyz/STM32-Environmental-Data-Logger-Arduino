#pragma once

#include <Arduino.h>
#include <Wire.h>

class AHT20
{
public:
    explicit AHT20(
        TwoWire &wire,
        uint8_t address = DEFAULT_ADDRESS);

    bool begin();

    bool read(
        int16_t &tempX100,  // Hundredths of °C: 2323 = 23.23 °C
        uint16_t &humX100); // Hundredths of %: 4880 = 48.80 %

private:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x38;

    static constexpr uint8_t CMD_INITIALIZE = 0xBE;
    static constexpr uint8_t CMD_TRIGGER = 0xAC;
    static constexpr uint8_t CMD_STATUS = 0x71;
    static constexpr uint8_t CMD_RESET = 0xBA;

    TwoWire &wire;
    uint8_t address;
    bool initialized;

    bool writeCommand(
        uint8_t command,
        uint8_t arg1,
        uint8_t arg2);

    bool readStatus(uint8_t &status);

    bool waitForMeasurement();

    uint8_t calculateCRC(
        const uint8_t *data,
        size_t length) const;
};

#pragma once

#include <Arduino.h>
#include <Wire.h>

class BMP280
{
public:
    explicit BMP280(
        TwoWire &wire,
        uint8_t address = DEFAULT_ADDRESS);

    bool begin();

    bool read(
        int16_t &tempX100,     // Hundredths of °C: 2401 = 24.01 °C
        uint32_t &pressurePa); // Pa: 100808 = 1008.08 hPa

    uint8_t getAddress() const;

    // Altitude in centimeters (table interpolation without pow).
    int32_t calculateAltitude(
        uint32_t pressurePa,
        uint32_t seaLevelPa = 101325UL) const;

private:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x77;
    static constexpr uint8_t ALTERNATIVE_ADDRESS = 0x76;

    static constexpr uint8_t REG_CHIP_ID = 0xD0;
    static constexpr uint8_t REG_RESET = 0xE0;
    static constexpr uint8_t REG_CTRL_MEAS = 0xF4;
    static constexpr uint8_t REG_CONFIG = 0xF5;
    static constexpr uint8_t REG_PRESS_MSB = 0xF7;
    static constexpr uint8_t REG_CALIB_START = 0x88;

    static constexpr uint8_t CHIP_ID = 0x58;

    TwoWire &wire;
    uint8_t address;
    bool initialized;

    uint16_t digT1;
    int16_t digT2;
    int16_t digT3;

    uint16_t digP1;
    int16_t digP2;
    int16_t digP3;
    int16_t digP4;
    int16_t digP5;
    int16_t digP6;
    int16_t digP7;
    int16_t digP8;
    int16_t digP9;

    int32_t tFine;

    bool detectSensor();

    bool readCalibration();

    bool readRegisters(
        uint8_t startRegister,
        uint8_t *buffer,
        uint8_t length);

    bool writeRegister(
        uint8_t reg,
        uint8_t value);

    uint8_t readRegister(
        uint8_t reg);

    int32_t compensateTemperature(
        uint32_t rawTemperature);

    uint32_t compensatePressure(
        uint32_t rawPressure);
};

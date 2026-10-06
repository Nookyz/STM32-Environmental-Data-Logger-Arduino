#pragma once

#include <Arduino.h>
#include <string.h>
#include "drivers/W25Q128.hpp"

// Offsets use the same units as the values:
// 0.01 °C for temperature, Pa for pressure
struct CalibrationData
{
  uint32_t magic;
  int32_t ahtTemperatureOffset;
  int32_t bmpTemperatureOffset;
  int32_t pressureOffset;
  uint16_t version;
  uint16_t crc; // CRC-16 of all previous bytes
};

static_assert(sizeof(CalibrationData) == 20, "Unexpected struct padding");

class CalibrationStorage
{
public:
  explicit CalibrationStorage(W25Q128 &flash, uint32_t address = 0)
      : flash(flash), address(address)
  {
  }

  // Returns false if flash is empty, corrupted or of another version
  bool load(CalibrationData &out)
  {
    CalibrationData data;

    if (!flash.read(address, reinterpret_cast<uint8_t *>(&data),
                    sizeof(data)))
    {
      return false;
    }

    if (data.magic != MAGIC || data.version != VERSION ||
        data.crc != calculateCrc(data))
    {
      return false;
    }

    out = data;
    return true;
  }

  // Erases the sector, writes the data and verifies it by reading back
  bool save(CalibrationData data)
  {
    data.magic = MAGIC;
    data.version = VERSION;
    data.crc = calculateCrc(data);

    if (!flash.eraseSector(address) ||
        !flash.write(address, reinterpret_cast<const uint8_t *>(&data),
                     sizeof(data)))
    {
      return false;
    }

    CalibrationData check;
    if (!flash.read(address, reinterpret_cast<uint8_t *>(&check),
                    sizeof(check)))
    {
      return false;
    }

    return memcmp(&data, &check, sizeof(data)) == 0;
  }

private:
  static constexpr uint32_t MAGIC = 0xCA11B001UL;
  static constexpr uint16_t VERSION = 1;

  // CRC-16/CCITT-FALSE
  static uint16_t calculateCrc(const CalibrationData &data)
  {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&data);
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < sizeof(data) - sizeof(data.crc); ++i)
    {
      crc ^= static_cast<uint16_t>(p[i]) << 8;

      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                             : static_cast<uint16_t>(crc << 1);
      }
    }

    return crc;
  }

  W25Q128 &flash;
  uint32_t address;
};
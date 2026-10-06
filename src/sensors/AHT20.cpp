#include "sensors/AHT20.hpp"
#include "utils/Timer.hpp"

namespace
{
  constexpr uint8_t CALIBRATION_BIT = 0x08;
  constexpr uint8_t BUSY_BIT = 0x80;

  constexpr uint8_t MEASUREMENT_SIZE = 7;

  constexpr uint8_t TRIGGER_ARG_1 = 0x33;
  constexpr uint8_t TRIGGER_ARG_2 = 0x00;

  constexpr uint8_t INITIALIZE_ARG_1 = 0x08;
  constexpr uint8_t INITIALIZE_ARG_2 = 0x00;

  constexpr uint32_t STARTUP_DELAY_MS = 100;
  constexpr uint32_t CALIBRATION_DELAY_MS = 10;
  constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 100;
  constexpr uint32_t STATUS_POLL_DELAY_MS = 5;

  uint32_t extractRawHumidity(const uint8_t *data)
  {
    return (static_cast<uint32_t>(data[1]) << 12) |
           (static_cast<uint32_t>(data[2]) << 4) |
           (data[3] >> 4);
  }

  uint32_t extractRawTemperature(const uint8_t *data)
  {
    return (static_cast<uint32_t>(data[3] & 0x0F) << 16) |
           (static_cast<uint32_t>(data[4]) << 8) |
           data[5];
  }

  // Humidity in hundredths of a percent:
  // raw * 10000 / 2^20.
  //
  // Use raw >> 4 (16 bits) so the multiplication fits into uint32_t.
  // 65535 * 10000 = 655,350,000.
  // The resulting resolution is approximately 0.0015%,
  // which is below the sensor noise level.
  uint16_t convertHumidity(uint32_t rawHumidity)
  {
    const uint32_t v = ((rawHumidity >> 4) * 10000UL + 32768UL) >> 16;
    return static_cast<uint16_t>(v > 10000UL ? 10000UL : v);
  }

  // Temperature in hundredths of a degree Celsius:
  // raw * 20000 / 2^20 - 5000.
  //
  // 65535 * 20000 = 1,310,700,000 < 2^32.
  // The resulting resolution is approximately 0.003 °C.
  int16_t convertTemperature(uint32_t rawTemperature)
  {
    const int32_t v =
        static_cast<int32_t>(
            ((rawTemperature >> 4) * 20000UL + 32768UL) >> 16) -
        5000;
    return static_cast<int16_t>(v);
  }
}

AHT20::AHT20(
    TwoWire &wire,
    uint8_t address)
    : wire(wire),
      address(address),
      initialized(false)
{
}

bool AHT20::writeCommand(
    uint8_t command,
    uint8_t arg1,
    uint8_t arg2)
{
  wire.beginTransmission(address);

  wire.write(command);
  wire.write(arg1);
  wire.write(arg2);

  return wire.endTransmission() == 0;
}

bool AHT20::readStatus(uint8_t &status)
{
  wire.beginTransmission(address);
  wire.write(CMD_STATUS);

  if (wire.endTransmission() != 0)
  {
    return false;
  }

  if (wire.requestFrom(
          address,
          static_cast<uint8_t>(1)) != 1)
  {
    return false;
  }

  status = wire.read();

  return true;
}

bool AHT20::begin()
{
  delay(STARTUP_DELAY_MS);

  uint8_t status = 0;

  if (!readStatus(status))
  {
    return false;
  }

  if ((status & CALIBRATION_BIT) != 0)
  {
    initialized = true;
    return true;
  }

  if (!writeCommand(
          CMD_INITIALIZE,
          INITIALIZE_ARG_1,
          INITIALIZE_ARG_2))
  {
    return false;
  }

  delay(CALIBRATION_DELAY_MS);

  if (!readStatus(status))
  {
    return false;
  }

  if ((status & CALIBRATION_BIT) == 0)
  {
    return false;
  }

  initialized = true;

  return true;
}

bool AHT20::waitForMeasurement()
{
  Timer timeoutTimer(MEASUREMENT_TIMEOUT_MS);

  timeoutTimer.reset();

  while (!timeoutTimer.expired())
  {
    uint8_t status = 0;

    if (!readStatus(status))
    {
      return false;
    }

    if ((status & BUSY_BIT) == 0)
    {
      return true;
    }

    delay(STATUS_POLL_DELAY_MS);
  }

  return false;
}

uint8_t AHT20::calculateCRC(
    const uint8_t *data,
    size_t length) const
{
  constexpr uint8_t CRC_INITIAL = 0xFF;
  constexpr uint8_t CRC_POLYNOMIAL = 0x31;

  uint8_t crc = CRC_INITIAL;

  for (size_t i = 0; i < length; ++i)
  {
    crc ^= data[i];

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
      if ((crc & 0x80) != 0)
      {
        crc = static_cast<uint8_t>((crc << 1) ^ CRC_POLYNOMIAL);
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}

bool AHT20::read(
    int16_t &tempX100,
    uint16_t &humX100)
{
  if (!initialized)
  {
    return false;
  }

  if (!writeCommand(
          CMD_TRIGGER,
          TRIGGER_ARG_1,
          TRIGGER_ARG_2))
  {
    return false;
  }

  if (!waitForMeasurement())
  {
    return false;
  }

  uint8_t measurementData[MEASUREMENT_SIZE];

  if (wire.requestFrom(
          address,
          MEASUREMENT_SIZE) != MEASUREMENT_SIZE)
  {
    return false;
  }

  for (uint8_t i = 0; i < MEASUREMENT_SIZE; ++i)
  {
    measurementData[i] = wire.read();
  }

  if (calculateCRC(measurementData, MEASUREMENT_SIZE - 1) !=
      measurementData[MEASUREMENT_SIZE - 1])
  {
    return false;
  }

  humX100 = convertHumidity(extractRawHumidity(measurementData));
  tempX100 = convertTemperature(extractRawTemperature(measurementData));

  return true;
}

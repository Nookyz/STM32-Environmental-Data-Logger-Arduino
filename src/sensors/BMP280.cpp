#include "sensors/BMP280.hpp"

namespace
{
  uint16_t readUInt16LE(const uint8_t *data)
  {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
  }

  int16_t readInt16LE(const uint8_t *data)
  {
    return static_cast<int16_t>(
        static_cast<uint16_t>(data[0]) |
        (static_cast<uint16_t>(data[1]) << 8));
  }

  // Altitude based on the standard atmosphere:
  // h = 44330 * (1 - (p / p0)^0.190295)
  //
  // Values are stored in decimeters for p/p0 from 1.050 to 0.500
  // with a step of 0.025.
  // Linear interpolation is used between table entries.
  // The error relative to the original formula is less than 0.5 m.
  constexpr uint32_t ALT_RATIO_TOP = 10500;   // 1.050 in units of 1/10000
  constexpr uint32_t ALT_RATIO_STEP = 250;    // 0.025
  constexpr uint32_t ALT_RATIO_BOTTOM = 5000; // 0.500

  constexpr int32_t ALT_TABLE_DM[] = {
      -4135, -2088, 0, 2131, 4306, 6528, 8799, 11124,
      13500, 15935, 18429, 20990, 23617, 26315, 29091, 31947,
      34892, 37928, 41064, 44310, 47670, 51157, 54781};

  constexpr uint8_t ALT_TABLE_SIZE =
      sizeof(ALT_TABLE_DM) / sizeof(ALT_TABLE_DM[0]);
}

BMP280::BMP280(
    TwoWire &wire,
    uint8_t address)
    : wire(wire),
      address(address),
      initialized(false),
      digT1(0),
      digT2(0),
      digT3(0),
      digP1(0),
      digP2(0),
      digP3(0),
      digP4(0),
      digP5(0),
      digP6(0),
      digP7(0),
      digP8(0),
      digP9(0),
      tFine(0)
{
}

uint8_t BMP280::getAddress() const
{
  return address;
}

uint8_t BMP280::readRegister(uint8_t reg)
{
  wire.beginTransmission(address);
  wire.write(reg);

  if (wire.endTransmission() != 0)
  {
    return 0;
  }

  if (wire.requestFrom(
          address,
          static_cast<uint8_t>(1)) != 1)
  {
    return 0;
  }

  return wire.read();
}

bool BMP280::writeRegister(
    uint8_t reg,
    uint8_t value)
{
  wire.beginTransmission(address);

  wire.write(reg);
  wire.write(value);

  return wire.endTransmission() == 0;
}

bool BMP280::readRegisters(
    uint8_t startRegister,
    uint8_t *buffer,
    uint8_t length)
{
  wire.beginTransmission(address);
  wire.write(startRegister);

  if (wire.endTransmission() != 0)
  {
    return false;
  }

  if (wire.requestFrom(
          address,
          length) != length)
  {
    return false;
  }

  for (uint8_t i = 0; i < length; ++i)
  {
    buffer[i] = wire.read();
  }

  return true;
}

bool BMP280::detectSensor()
{
  return readRegister(REG_CHIP_ID) == CHIP_ID;
}

bool BMP280::readCalibration()
{
  uint8_t calibrationData[24];

  if (!readRegisters(
          REG_CALIB_START,
          calibrationData,
          sizeof(calibrationData)))
  {
    return false;
  }

  // Temperature: digT1 is unsigned, digT2..digT3 are signed.
  digT1 = readUInt16LE(&calibrationData[0]);
  digT2 = readInt16LE(&calibrationData[2]);
  digT3 = readInt16LE(&calibrationData[4]);

  // Pressure: digP1 is unsigned, digP2..digP9 are signed.
  digP1 = readUInt16LE(&calibrationData[6]);
  digP2 = readInt16LE(&calibrationData[8]);
  digP3 = readInt16LE(&calibrationData[10]);
  digP4 = readInt16LE(&calibrationData[12]);
  digP5 = readInt16LE(&calibrationData[14]);
  digP6 = readInt16LE(&calibrationData[16]);
  digP7 = readInt16LE(&calibrationData[18]);
  digP8 = readInt16LE(&calibrationData[20]);
  digP9 = readInt16LE(&calibrationData[22]);

  // digP1 must not be zero.
  return digP1 != 0;
}

bool BMP280::begin()
{
  // Try the configured address first.
  if (!detectSensor())
  {
    // Try the alternative BMP280 address.
    address =
        (address == DEFAULT_ADDRESS)
            ? ALTERNATIVE_ADDRESS
            : DEFAULT_ADDRESS;

    if (!detectSensor())
    {
      return false;
    }
  }

  // Perform a software reset.
  if (!writeRegister(REG_RESET, 0xB6))
  {
    return false;
  }

  delay(5);

  if (!readCalibration())
  {
    return false;
  }

  // CTRL_MEAS = 0x27: temperature x1, pressure x1, normal mode.
  if (!writeRegister(REG_CTRL_MEAS, 0x27))
  {
    return false;
  }

  // CONFIG = 0x00: IIR filter disabled.
  if (!writeRegister(REG_CONFIG, 0x00))
  {
    return false;
  }

  initialized = true;

  return true;
}

int32_t BMP280::compensateTemperature(
    uint32_t rawTemperature)
{
  const int32_t adcTemperature =
      static_cast<int32_t>(rawTemperature);

  const int32_t temperatureDifference =
      (adcTemperature >> 3) -
      (static_cast<int32_t>(digT1) << 1);

  const int32_t temperatureCompensation1 =
      (temperatureDifference * digT2) >> 11;

  const int32_t temperatureOffset =
      (adcTemperature >> 4) -
      static_cast<int32_t>(digT1);

  int32_t temperatureCompensation2 =
      (temperatureOffset * temperatureOffset) >> 12;

  temperatureCompensation2 =
      (temperatureCompensation2 * digT3) >> 14;

  // tFine is required for pressure compensation.
  tFine =
      temperatureCompensation1 +
      temperatureCompensation2;

  // Result: hundredths of a degree Celsius.
  return (tFine * 5 + 128) >> 8;
}

uint32_t BMP280::compensatePressure(
    uint32_t rawPressure)
{
  // Bosch compensation formula.
  // Intermediate calculations require int64_t.
  int64_t pressureCompensation1 =
      static_cast<int64_t>(tFine) - 128000;

  int64_t pressureCompensation2 =
      pressureCompensation1 *
      pressureCompensation1 *
      digP6;

  pressureCompensation2 +=
      (pressureCompensation1 *
       digP5)
      << 17;

  pressureCompensation2 +=
      static_cast<int64_t>(digP4) << 35;

  pressureCompensation1 =
      ((pressureCompensation1 *
        pressureCompensation1 *
        digP3) >>
       8) +
      ((pressureCompensation1 *
        digP2)
       << 12);

  pressureCompensation1 =
      ((((static_cast<int64_t>(1) << 47) +
         pressureCompensation1) *
        digP1) >>
       33);

  if (pressureCompensation1 == 0)
  {
    return 0;
  }

  int64_t pressure =
      1048576 -
      static_cast<int64_t>(rawPressure);

  pressure =
      (((pressure << 31) -
        pressureCompensation2) *
       3125) /
      pressureCompensation1;

  pressureCompensation1 =
      (static_cast<int64_t>(digP9) *
       (pressure >> 13) *
       (pressure >> 13)) >>
      25;

  pressureCompensation2 =
      (static_cast<int64_t>(digP8) *
       pressure) >>
      19;

  pressure =
      ((pressure +
        pressureCompensation1 +
        pressureCompensation2) >>
       8) +
      (static_cast<int64_t>(digP7) << 4);

  // Convert from Q24.8 fixed-point format to pascals.
  return static_cast<uint32_t>(pressure >> 8);
}

bool BMP280::read(
    int16_t &tempX100,
    uint32_t &pressurePa)
{
  if (!initialized)
  {
    return false;
  }

  // Read pressure and temperature in a single I2C transaction:
  // registers 0xF7 ... 0xFC.
  uint8_t measurementData[6];

  if (!readRegisters(
          REG_PRESS_MSB,
          measurementData,
          sizeof(measurementData)))
  {
    return false;
  }

  const uint32_t rawPressure =
      (static_cast<uint32_t>(measurementData[0]) << 12) |
      (static_cast<uint32_t>(measurementData[1]) << 4) |
      (measurementData[2] >> 4);

  const uint32_t rawTemperature =
      (static_cast<uint32_t>(measurementData[3]) << 12) |
      (static_cast<uint32_t>(measurementData[4]) << 4) |
      (measurementData[5] >> 4);

  // Calculate temperature first because pressure compensation uses tFine.
  const int32_t temperature = compensateTemperature(rawTemperature);
  const uint32_t pressure = compensatePressure(rawPressure);

  if (pressure == 0)
  {
    return false;
  }

  tempX100 = static_cast<int16_t>(temperature);
  pressurePa = pressure;

  return true;
}

// Calculate altitude in centimeters using table interpolation instead of pow().
int32_t BMP280::calculateAltitude(
    uint32_t pressurePa,
    uint32_t seaLevelPa) const
{
  if (pressurePa == 0 || seaLevelPa == 0)
  {
    return 0;
  }

  // Pressure ratio p/p0 in units of 1/10000.
  // 110000 * 10000 fits into uint32_t.
  uint32_t ratio = (pressurePa * 10000UL) / seaLevelPa;

  // Clamp values outside the table range
  // (approximately -410 m ... +5480 m).
  if (ratio >= ALT_RATIO_TOP)
  {
    return ALT_TABLE_DM[0] * 10;
  }
  if (ratio <= ALT_RATIO_BOTTOM)
  {
    return ALT_TABLE_DM[ALT_TABLE_SIZE - 1] * 10;
  }

  const uint32_t pos = ALT_RATIO_TOP - ratio;
  const uint8_t i = static_cast<uint8_t>(pos / ALT_RATIO_STEP);
  const int32_t frac = static_cast<int32_t>(pos % ALT_RATIO_STEP);

  const int32_t h0 = ALT_TABLE_DM[i];
  const int32_t h1 = ALT_TABLE_DM[i + 1];

  // Convert decimeters to centimeters.
  return (h0 + (h1 - h0) * frac / static_cast<int32_t>(ALT_RATIO_STEP)) * 10;
}

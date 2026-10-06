#pragma once

#include <Arduino.h>

#include "drivers/Led.hpp"
#include "sensors/AHT20.hpp"
#include "sensors/BMP280.hpp"

#include "storage/CalibrationStorage.hpp"

#include "utils/Timer.hpp"
#include "utils/MovingAverage.hpp"
#include "utils/Calibration.hpp"
#include "utils/DataValidator.hpp"

class Command
{
public:
  Command(
      Led &led,
      AHT20 &aht20,
      BMP280 &bmp280,
      CalibrationStorage &calibrationStorage);

  void update();

  // Load calibration from Flash
  void setCalibration(const CalibrationData &cal);

private:
  static constexpr uint8_t BUFFER_SIZE = 32;
  static constexpr uint32_t WATCH_INTERVAL_MS = 2000;

  // UART command parser
  bool lastWasCR;
  char buffer[BUFFER_SIZE];
  uint8_t bufferIndex;

  // Hardware
  Led &led;
  AHT20 &aht20;
  BMP280 &bmp280;
  CalibrationStorage &calibrationStorage;

  // WATCH
  Timer watchTimer;
  bool watchEnabled;

  // Command processing
  void processCommand(const char *command);

  // Sensor output
  void printSensorData();

  // Calibration
  void printCalibration();
  bool calibrateSensors();
  bool saveCalibration();

  CalibrationData getCalibration() const;

  // Filters
  MovingAverage<int32_t, 5> ahtTemperatureFilter;
  MovingAverage<int32_t, 5> bmpTemperatureFilter;
  MovingAverage<int32_t, 5> pressureFilter;

  // Calibration
  Calibration<int32_t> ahtTemperatureCalibration;
  Calibration<int32_t> bmpTemperatureCalibration;
  Calibration<int32_t> pressureCalibration;

  // Validation
  DataValidator<int32_t> ahtTemperatureValidator{-4000, 8500, 500};
  DataValidator<int32_t> humidityValidator{0, 10000, 2000};
  DataValidator<int32_t> bmpTemperatureValidator{-4000, 8500, 500};
  DataValidator<int32_t> pressureValidator{30000, 110000, 500};
};

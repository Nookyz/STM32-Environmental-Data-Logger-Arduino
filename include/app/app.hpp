#pragma once

#include "communication/Command.hpp"
#include "drivers/Button.hpp"
#include "drivers/Led.hpp"
#include "drivers/W25Q128.hpp"
#include "sensors/AHT20.hpp"
#include "sensors/BMP280.hpp"
#include "storage/CalibrationStorage.hpp"
#include "utils/ClickDetector.hpp"

class App
{
public:
  App();

  void begin();
  void update();

private:
  void handleClicks(uint8_t clicks);

  static App *instance;
  static void buttonISR();

  volatile bool buttonPressed = false;

  Button button;
  Led led;
  ClickDetector clickDetector;

  AHT20 aht20;
  BMP280 bmp280;

  W25Q128 flash;
  CalibrationStorage calibrationStorage;

  Command command;
};
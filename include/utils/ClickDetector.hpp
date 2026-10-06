#pragma once

#include <Arduino.h>
#include "utils/Timer.hpp"

class ClickDetector
{
private:
  Timer clickTimer;
  uint8_t clickCount;

public:
  ClickDetector(uint16_t clickTimeout = 500);

  void click();
  uint8_t update();
};
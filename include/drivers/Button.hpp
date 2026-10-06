#pragma once

#include <Arduino.h>
#include "utils/Timer.hpp"

class Button
{
public:
  Button(uint8_t pin, uint16_t debounceTime = 50);

  void begin();
  void update();

  bool isPressed();
  bool wasPressed();

private:
  uint8_t pin;
  uint16_t debounceTime;

  bool stableState;
  bool pressedEvent;

  volatile bool interruptTriggered;
  bool debounceActive;

  Timer debounceTimer;

  static Button *instance;
  static void interruptHandler();
};
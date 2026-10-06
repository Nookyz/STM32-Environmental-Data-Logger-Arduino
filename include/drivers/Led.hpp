#pragma once

#include <Arduino.h>
#include "utils/Timer.hpp"

class Led
{
public:
  enum class Mode : uint8_t
  {
    OFF,
    ON,
    SLOW_BLINK,
    FAST_BLINK
  };

private:
  const uint8_t pin;

  Mode mode;
  Timer blinkTimer;

  static constexpr unsigned long SLOW_INTERVAL = 500;
  static constexpr unsigned long FAST_INTERVAL = 100;

public:
  Led(uint8_t pin);

  void begin();

  void on();
  void off();
  bool isOn();

  void setMode(Mode newMode);
  void nextMode();
  void update();

  Mode getMode();
};
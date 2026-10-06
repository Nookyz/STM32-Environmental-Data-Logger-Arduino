#pragma once

#include <Arduino.h>

class Timer
{
private:
  unsigned long startTime;
  unsigned long interval;

public:
  Timer(unsigned long interval = 0);

  void start();
  void reset();

  void setInterval(unsigned long interval);

  bool expired();
};
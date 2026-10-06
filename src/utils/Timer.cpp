#include "utils/Timer.hpp"

Timer::Timer(unsigned long interval)
    : startTime(0),
      interval(interval)
{
}

void Timer::start()
{
  startTime = millis();
}

void Timer::reset()
{
  startTime = millis();
}

void Timer::setInterval(unsigned long interval)
{
  this->interval = interval;
}

bool Timer::expired()
{
  return millis() - startTime >= interval;
}
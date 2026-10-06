#include "utils/ClickDetector.hpp"

ClickDetector::ClickDetector(uint16_t clickTimeout)
    : clickTimer(clickTimeout),
      clickCount(0)
{
}

void ClickDetector::click()
{
  clickTimer.reset();

  if (clickCount < 255)
  {
    clickCount++;
  }
}

uint8_t ClickDetector::update()
{
  if (clickCount == 0)
  {
    return 0;
  }

  if (!clickTimer.expired())
  {
    return 0;
  }

  uint8_t result = clickCount;

  clickCount = 0;

  return result;
}
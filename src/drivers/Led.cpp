#include "drivers/Led.hpp"

Led::Led(uint8_t pin)
    : pin(pin),
      mode(Mode::OFF),
      blinkTimer(0)
{
}

void Led::begin()
{
  pinMode(pin, OUTPUT);
  off();
}

void Led::on()
{
  digitalWrite(pin, HIGH);
}

void Led::off()
{
  digitalWrite(pin, LOW);
}

bool Led::isOn()
{
  return digitalRead(pin) == HIGH;
}

void Led::setMode(Mode newMode)
{
  mode = newMode;

  switch (mode)
  {
  case Mode::OFF:
    off();
    break;

  case Mode::ON:
    on();
    break;

  case Mode::SLOW_BLINK:
    on();
    blinkTimer.setInterval(SLOW_INTERVAL);
    blinkTimer.reset();
    break;

  case Mode::FAST_BLINK:
    on();
    blinkTimer.setInterval(FAST_INTERVAL);
    blinkTimer.reset();
    break;
  }
}

void Led::nextMode()
{
  switch (mode)
  {
  case Mode::OFF:
    setMode(Mode::ON);
    break;

  case Mode::ON:
    setMode(Mode::SLOW_BLINK);
    break;

  case Mode::SLOW_BLINK:
    setMode(Mode::FAST_BLINK);
    break;

  case Mode::FAST_BLINK:
    setMode(Mode::OFF);
    break;
  }
}

void Led::update()
{
  if (mode != Mode::SLOW_BLINK &&
      mode != Mode::FAST_BLINK)
  {
    return;
  }

  if (blinkTimer.expired())
  {
    blinkTimer.reset();

    if (isOn())
      off();
    else
      on();
  }
}

Led::Mode Led::getMode()
{
  return mode;
}
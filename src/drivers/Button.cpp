#include "drivers/Button.hpp"

Button *Button::instance = nullptr;

Button::Button(uint8_t pin, uint16_t debounceTime)
    : pin(pin),
      debounceTime(debounceTime),
      stableState(HIGH),
      pressedEvent(false),
      interruptTriggered(false),
      debounceActive(false),
      debounceTimer(debounceTime)
{
}

void Button::begin()
{
  instance = this;

  pinMode(pin, INPUT_PULLUP);

  attachInterrupt(
      digitalPinToInterrupt(pin),
      Button::interruptHandler,
      CHANGE);
}

void Button::interruptHandler()
{
  if (instance != nullptr)
  {
    instance->interruptTriggered = true;
  }
}

void Button::update()
{
  // An interrupt was triggered.
  if (interruptTriggered)
  {
    interruptTriggered = false;

    // Ignore the interrupt if debounce is already active.
    if (!debounceActive)
    {
      debounceTimer.reset();
      debounceActive = true;
    }
  }

  // Debounce period has not finished yet.
  if (!debounceActive)
  {
    return;
  }

  if (!debounceTimer.expired())
  {
    return;
  }

  // Debounce period has finished.
  debounceActive = false;

  // Read the actual button state.
  bool reading = digitalRead(pin);

  if (reading == LOW && stableState == HIGH)
  {
    stableState = LOW;

    // A confirmed button press occurred.
    pressedEvent = true;
  }
  else if (reading == HIGH)
  {
    stableState = HIGH;
  }
}

bool Button::isPressed()
{
  return stableState == LOW;
}

bool Button::wasPressed()
{
  if (!pressedEvent)
  {
    return false;
  }

  pressedEvent = false;

  return true;
}

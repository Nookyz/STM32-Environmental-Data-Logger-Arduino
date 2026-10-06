#pragma once

#include <stdint.h>

enum class ValidationResult : uint8_t
{
  Ok,         // Value accepted
  OutOfRange, // Outside [min, max], always rejected
  Spike,      // Jump from last accepted value is too large, rejected
  Rebased     // Too many consecutive spikes: accepted as the new baseline
};

// Range and spike check for integer sensor values.
// T is the value type, units are whatever the caller uses
// (0.01 °C for temperature, Pa for pressure).
template <typename T>
class DataValidator
{
public:
  constexpr DataValidator(T minValue, T maxValue, T maxDelta,
                          uint8_t maxRejects = 3)
      : minValue(minValue),
        maxValue(maxValue),
        maxDelta(maxDelta),
        maxRejects(maxRejects)
  {
  }

  ValidationResult check(T value)
  {
    if (value < minValue || value > maxValue)
    {
      return ValidationResult::OutOfRange;
    }

    // The first sample has nothing to compare with: only the range check
    if (!hasLast)
    {
      accept(value);
      return ValidationResult::Ok;
    }

    const int32_t delta = static_cast<int32_t>(value) -
                          static_cast<int32_t>(last);
    const int32_t absDelta = (delta < 0) ? -delta : delta;

    if (absDelta <= static_cast<int32_t>(maxDelta))
    {
      accept(value);
      return ValidationResult::Ok;
    }

    // Large jump: reject it, but if it persists, the real value has
    // probably changed (e.g. sensor moved), so take it as a new baseline
    if (++rejects >= maxRejects)
    {
      accept(value);
      return ValidationResult::Rebased;
    }

    return ValidationResult::Spike;
  }

  static constexpr bool isValid(ValidationResult r)
  {
    return r == ValidationResult::Ok || r == ValidationResult::Rebased;
  }

  void reset()
  {
    hasLast = false;
    rejects = 0;
  }

private:
  void accept(T value)
  {
    last = value;
    hasLast = true;
    rejects = 0;
  }

  T minValue;
  T maxValue;
  T maxDelta;
  T last = 0;
  uint8_t maxRejects;
  uint8_t rejects = 0;
  bool hasLast = false;
};
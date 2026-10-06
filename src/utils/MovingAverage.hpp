#pragma once

#include <stdint.h>

// Moving average filter without floating-point arithmetic.
//
// T   - value type
// N   - window size (1..255)
// Acc - accumulator type (int32_t by default) used to prevent
//       sum overflow, even when T is int16_t.
template <typename T, uint8_t N, typename Acc = int32_t>
class MovingAverage
{
  static_assert(N > 0, "Window size must be > 0");

public:
  MovingAverage() { reset(); }

  T update(T value)
  {
    if (count < N)
    {
      ++count;
    }
    else
    {
      sum -= values[index];
    }

    values[index] = value;
    sum += value;

    if (++index >= N)
    {
      index = 0;
    }

    return getAverage();
  }

  // Returns the average rounded to the nearest integer
  // instead of truncating towards zero.
  T getAverage() const
  {
    if (count == 0)
    {
      return 0;
    }

    const Acc half = static_cast<Acc>(count / 2);
    return static_cast<T>(
        (sum >= 0 ? (sum + half) : (sum - half)) / static_cast<Acc>(count));
  }

  void reset()
  {
    index = 0;
    count = 0;
    sum = 0;

    for (uint8_t i = 0; i < N; ++i)
    {
      values[i] = 0;
    }
  }

private:
  T values[N];
  Acc sum;
  uint8_t index;
  uint8_t count;
};

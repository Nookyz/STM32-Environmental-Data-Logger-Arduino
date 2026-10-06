#pragma once

// Offset-based calibration.
//
// For int32_t, the calibration units are the same as the value units:
// temperature is measured in hundredths of a degree Celsius
// (-50 = -0.50 °C),
// pressure is measured in pascals
// (-250 = -2.50 hPa).
template <typename T>
class Calibration
{
public:
  constexpr explicit Calibration(T offset = 0) : offset(offset) {}

  constexpr T apply(T value) const { return value + offset; }

  void setOffset(T value) { offset = value; }

  constexpr T getOffset() const { return offset; }

  void reset() { offset = 0; }

private:
  T offset;
};

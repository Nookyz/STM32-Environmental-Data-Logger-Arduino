#include <Arduino.h>

#ifdef UNITY_INCLUDE_CONFIG_H
#undef UNITY_INCLUDE_CONFIG_H
#endif

#include <unity.h>

#include "drivers/Led.hpp"

namespace
{
  constexpr uint8_t LED_PIN = PB1;

  constexpr uint16_t SLOW_BLINK_DELAY_MS = 550;
  constexpr uint16_t FAST_BLINK_DELAY_MS = 150;
  constexpr uint16_t OFF_DELAY_MS = 600;

  Led led(LED_PIN);

  void assertLedMode(Led::Mode expected)
  {
    TEST_ASSERT_EQUAL(
        static_cast<int>(expected),
        static_cast<int>(led.getMode()));
  }

  void assertLedState(uint8_t expectedState)
  {
    TEST_ASSERT_EQUAL(expectedState, digitalRead(LED_PIN));
  }
}

void setUp()
{
  led.begin();
}

void tearDown()
{
  // Always leave the LED in a known state after each test.
  led.setMode(Led::Mode::OFF);
}

void test_led_off()
{
  led.setMode(Led::Mode::OFF);

  assertLedMode(Led::Mode::OFF);
  assertLedState(LOW);
}

void test_led_on()
{
  led.setMode(Led::Mode::ON);

  assertLedMode(Led::Mode::ON);
  assertLedState(HIGH);
}

void test_led_slow_blink()
{
  led.setMode(Led::Mode::SLOW_BLINK);

  // The LED should turn on immediately after entering blink mode.
  assertLedMode(Led::Mode::SLOW_BLINK);
  assertLedState(HIGH);

  // Wait until the slow blink interval has elapsed.
  delay(SLOW_BLINK_DELAY_MS);

  led.update();

  // The LED should toggle off.
  assertLedState(LOW);

  delay(SLOW_BLINK_DELAY_MS);

  led.update();

  // The LED should toggle on again.
  assertLedState(HIGH);
}

void test_led_fast_blink()
{
  led.setMode(Led::Mode::FAST_BLINK);

  // The LED should turn on immediately after entering blink mode.
  assertLedMode(Led::Mode::FAST_BLINK);
  assertLedState(HIGH);

  // Wait until the fast blink interval has elapsed.
  delay(FAST_BLINK_DELAY_MS);

  led.update();

  // The LED should toggle off.
  assertLedState(LOW);

  delay(FAST_BLINK_DELAY_MS);

  led.update();

  // The LED should toggle on again.
  assertLedState(HIGH);
}

void test_blink_to_off()
{
  led.setMode(Led::Mode::SLOW_BLINK);

  delay(SLOW_BLINK_DELAY_MS);
  led.update();

  // Switching from blink mode to OFF should immediately turn the LED off.
  led.setMode(Led::Mode::OFF);

  assertLedMode(Led::Mode::OFF);
  assertLedState(LOW);

  // update() must not turn the LED on again after switching to OFF.
  delay(OFF_DELAY_MS);
  led.update();

  assertLedState(LOW);
}

void setup()
{
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_led_off);
  RUN_TEST(test_led_on);
  RUN_TEST(test_led_slow_blink);
  RUN_TEST(test_led_fast_blink);
  RUN_TEST(test_blink_to_off);

  UNITY_END();
}

void loop()
{
}
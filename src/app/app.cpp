#include "app/app.hpp"

#include "drivers/W25Q128.hpp"
#include "storage/CalibrationStorage.hpp"

#define BUTTON_PIN PB0 // INPUT_PULLUP, button to GND
#define LED_PIN PB1    // Through a 330 Ohm...1 kOhm resistor

#define PIN_SCL PB6
#define PIN_SDA PB7

constexpr uint8_t PIN_FLASH_CS = PA4;

constexpr uint16_t CLICK_TIMEOUT = 500;
constexpr uint16_t DEBOUNCE_TIME = 50;

App::App()
    : button(BUTTON_PIN, DEBOUNCE_TIME),
      led(LED_PIN),
      clickDetector(CLICK_TIMEOUT),
      aht20(Wire),
      bmp280(Wire),
      flash(SPI, PIN_FLASH_CS),
      calibrationStorage(flash),
      command(
          led,
          aht20,
          bmp280,
          calibrationStorage)
{
}

void App::begin()
{
  Serial2.begin(115200);

  Wire.setSDA(PIN_SDA);
  Wire.setSCL(PIN_SCL);
  Wire.begin();

  led.begin();
  button.begin();

  // Initialize external Flash memory.
  if (flash.begin())
  {
    Serial2.println("FLASH: OK");

    CalibrationData calibration;

    if (calibrationStorage.load(calibration))
    {
      command.setCalibration(calibration);
      Serial2.println("CAL: loaded");
    }
    else
    {
      Serial2.println(
          "CAL: empty or invalid, using defaults");
    }
  }
  else
  {
    Serial2.println("FLASH: ERROR");
  }

  // Initialize AHT20 sensor.
  if (aht20.begin())
  {
    Serial2.println("AHT20: OK");
  }
  else
  {
    Serial2.println("AHT20: ERROR");
  }

  // Initialize BMP280 sensor.
  if (bmp280.begin())
  {
    Serial2.println("BMP280: OK");
  }
  else
  {
    Serial2.println("BMP280: ERROR");
  }
}

void App::update()
{
  // Button uses an interrupt with timer-based debounce.
  button.update();

  // Check whether a confirmed button press occurred.
  if (button.wasPressed())
  {
    Serial2.println("BUTTON INTERRUPT!");
    clickDetector.click();
  }

  // Detect single, double, or triple clicks.
  uint8_t clicks = clickDetector.update();

  handleClicks(clicks);

  // Update LED state.
  led.update();

  // Process UART commands.
  command.update();
}

void App::handleClicks(uint8_t clicks)
{
  if (clicks == 0)
  {
    return;
  }

  switch (clicks)
  {
  case 1:

    if (led.getMode() == Led::Mode::OFF)
    {
      led.setMode(Led::Mode::ON);
      Serial2.println("MODE: ON");
    }
    else
    {
      led.setMode(Led::Mode::OFF);
      Serial2.println("MODE: OFF");
    }

    break;

  case 2:

    led.setMode(Led::Mode::SLOW_BLINK);
    Serial2.println("MODE: SLOW BLINK");

    break;

  default:

    led.setMode(Led::Mode::FAST_BLINK);
    Serial2.println("MODE: FAST BLINK");

    break;
  }
}

#include "communication/Command.hpp"
#include "sensors/AHT20.hpp"
#include "sensors/BMP280.hpp"
#include "utils/ConsoleTable.hpp"
#include <cstring>

namespace
{
  using Table = ConsoleTable<2U, 16U, 192U>;
  using Validator = DataValidator<int32_t>;

  enum class CommandType : uint8_t
  {
    ON,
    OFF,
    SLOW,
    FAST,
    STATUS,
    READ,
    WATCH,
    CAL,
    SAVE,
    HELP,
    UNKNOWN
  };

  // Single source of truth for both the parser and HELP
  struct CommandDefinition
  {
    const char *name;
    const char *help;
    CommandType type;
  };

  constexpr CommandDefinition COMMANDS[] = {
      {"ON", "turn LED on", CommandType::ON},
      {"OFF", "turn LED off", CommandType::OFF},
      {"SLOW", "slow blink", CommandType::SLOW},
      {"FAST", "fast blink", CommandType::FAST},
      {"STATUS", "show current mode", CommandType::STATUS},
      {"READ", "read sensor data", CommandType::READ},
      {"WATCH", "watch sensor data", CommandType::WATCH},
      {"CAL", "calibrate sensors and save to flash", CommandType::CAL},
      {"SAVE", "save current calibration to flash", CommandType::SAVE},
      {"HELP", "show this help", CommandType::HELP}};

  constexpr uint8_t COMMAND_COUNT =
      sizeof(COMMANDS) / sizeof(COMMANDS[0]);

  CommandType findCommand(const char *name)
  {
    for (uint8_t i = 0; i < COMMAND_COUNT; ++i)
    {
      if (strcmp(name, COMMANDS[i].name) == 0)
      {
        return COMMANDS[i].type;
      }
    }

    return CommandType::UNKNOWN;
  }

  const char *ledModeName(Led::Mode mode)
  {
    switch (mode)
    {
    case Led::Mode::OFF:
      return "OFF";

    case Led::Mode::ON:
      return "ON";

    case Led::Mode::SLOW_BLINK:
      return "SLOW BLINK";

    case Led::Mode::FAST_BLINK:
      return "FAST BLINK";
    }

    return "?";
  }

  void applyLedMode(Led &led, Led::Mode mode)
  {
    led.setMode(mode);

    Serial2.print("MODE: ");
    Serial2.println(ledModeName(mode));
  }

  // Fixed-point values:
  //
  // temperature: 0.01 °C
  // humidity:    0.01 %
  // pressure:    Pa
  //
  // Example:
  // 2537 = 25.37 °C
  // 100812 = 1008.12 hPa
  struct Reading
  {
    int32_t raw;
    int32_t filtered;
    int32_t calibrated;
  };

  // Pipeline:
  //
  // raw
  //   ↓
  // validation
  //   ↓
  // filter
  //   ↓
  // calibration
  //
  // Returns false if the value was rejected.
  template <typename Filter, typename Cal, typename Val>
  bool process(
      Filter &filter,
      Cal &calibration,
      Val &validator,
      int32_t raw,
      Reading &out)
  {
    const ValidationResult result =
        validator.check(raw);

    if (!Val::isValid(result))
    {
      return false;
    }

    // Sensor level changed significantly.
    // Drop old filter history.
    if (result == ValidationResult::Rebased)
    {
      filter.reset();
    }

    const int32_t filtered =
        filter.update(raw);

    out = {
        raw,
        filtered,
        calibration.apply(filtered)};

    return true;
  }

  void addTemperatureRows(
      Table &t,
      const Reading &r)
  {
    t.addRow(
        "Temperature raw",
        t.numFixed(r.raw, 2, "°C"));

    t.addRow(
        "Temperature filtered",
        t.numFixed(r.filtered, 2, "°C"));

    t.addRow(
        "Temperature calibrated",
        t.numFixed(r.calibrated, 2, "°C"));
  }

  void printHelp()
  {
    Table t(Serial2);

    t.setTitle("Available commands")
        .setHeader("Command", "Description");

    for (uint8_t i = 0; i < COMMAND_COUNT; ++i)
    {
      t.addRow(
          COMMANDS[i].name,
          COMMANDS[i].help);
    }

    t.print();
  }
}

// ============================================================
// Constructor
// ============================================================

Command::Command(
    Led &led,
    AHT20 &aht20,
    BMP280 &bmp280,
    CalibrationStorage &calibrationStorage)
    : lastWasCR(false),
      led(led),
      aht20(aht20),
      bmp280(bmp280),
      calibrationStorage(calibrationStorage),
      bufferIndex(0),
      watchTimer(WATCH_INTERVAL_MS),
      watchEnabled(false)
{
}

// ============================================================
// UART update
// ============================================================

void Command::update()
{
  while (Serial2.available() > 0)
  {
    const int rx = Serial2.read();

    if (rx < 0)
    {
      break;
    }

    char c = static_cast<char>(rx);

    // --------------------------------------------------------
    // ENTER
    // CR / LF / CRLF
    // --------------------------------------------------------

    if (c == '\r' || c == '\n')
    {
      // Ignore LF directly following CR
      if (c == '\n' && lastWasCR)
      {
        lastWasCR = false;
        continue;
      }

      lastWasCR = (c == '\r');

      Serial2.println();

      if (bufferIndex > 0)
      {
        buffer[bufferIndex] = '\0';

        processCommand(buffer);

        bufferIndex = 0;
      }

      Serial2.print("Input: ");

      continue;
    }

    lastWasCR = false;

    // --------------------------------------------------------
    // BACKSPACE
    // --------------------------------------------------------

    if (c == '\b' || c == 127)
    {
      if (bufferIndex > 0)
      {
        --bufferIndex;

        Serial2.write("\b \b");
      }

      continue;
    }

    // --------------------------------------------------------
    // Printable ASCII only
    // --------------------------------------------------------

    if (c < ' ' || c > '~')
    {
      continue;
    }

    // --------------------------------------------------------
    // Convert lowercase to uppercase
    // --------------------------------------------------------

    if (c >= 'a' && c <= 'z')
    {
      c = static_cast<char>(
          c - 'a' + 'A');
    }

    // --------------------------------------------------------
    // Store command
    // --------------------------------------------------------

    if (bufferIndex < BUFFER_SIZE - 1)
    {
      buffer[bufferIndex++] = c;

      Serial2.write(c);
    }
  }

  // ----------------------------------------------------------
  // WATCH
  // ----------------------------------------------------------

  if (watchEnabled &&
      watchTimer.expired())
  {
    printSensorData();

    watchTimer.reset();
  }
}

// ============================================================
// Command processing
// ============================================================

void Command::processCommand(
    const char *command)
{
  switch (findCommand(command))
  {
  case CommandType::ON:

    applyLedMode(
        led,
        Led::Mode::ON);

    break;

  case CommandType::OFF:

    applyLedMode(
        led,
        Led::Mode::OFF);

    break;

  case CommandType::SLOW:

    applyLedMode(
        led,
        Led::Mode::SLOW_BLINK);

    break;

  case CommandType::FAST:

    applyLedMode(
        led,
        Led::Mode::FAST_BLINK);

    break;

  case CommandType::STATUS:

    Serial2.print("STATUS: ");

    Serial2.println(
        ledModeName(
            led.getMode()));

    break;

  case CommandType::READ:

    printSensorData();

    break;

  case CommandType::WATCH:

    watchEnabled = !watchEnabled;

    watchTimer.reset();

    Serial2.println(
        watchEnabled
            ? "WATCH: ON"
            : "WATCH: OFF");

    break;

  case CommandType::CAL:

    calibrateSensors();

    break;

  case CommandType::SAVE:

    if (saveCalibration())
    {
      Serial2.println(
          "CAL: saved");
    }
    else
    {
      Serial2.println(
          "CAL: SAVE ERROR");
    }

    break;

  case CommandType::HELP:

    printHelp();

    break;

  case CommandType::UNKNOWN:

    Serial2.println(
        "Unknown command. Type HELP");

    break;
  }
}

// ============================================================
// READ / WATCH
// ============================================================

void Command::printSensorData()
{
  int16_t ahtTemp = 0;
  uint16_t humidity = 0;

  int16_t bmpTemp = 0;
  uint32_t pressure = 0;

  const bool ahtOk =
      aht20.read(
          ahtTemp,
          humidity);

  const bool bmpOk =
      bmp280.read(
          bmpTemp,
          pressure);

  Table t(Serial2);

  t.setTitle("Sensor data")
      .setAlign(
          1,
          Table::Align::Right);

  // ==========================================================
  // AHT20
  // ==========================================================

  t.addSection("AHT20");

  if (!ahtOk)
  {
    t.addRow(
        "Status",
        "ERROR");
  }
  else
  {
    Reading temp{};

    if (process(
            ahtTemperatureFilter,
            ahtTemperatureCalibration,
            ahtTemperatureValidator,
            ahtTemp,
            temp))
    {
      addTemperatureRows(
          t,
          temp);
    }
    else
    {
      t.addRow(
          "Temperature",
          "INVALID");
    }

    if (Validator::isValid(
            humidityValidator.check(
                humidity)))
    {
      t.addRow(
          "Humidity",
          t.numFixed(
              humidity,
              2,
              "%"));
    }
    else
    {
      t.addRow(
          "Humidity",
          "INVALID");
    }
  }

  // ==========================================================
  // BMP280
  // ==========================================================

  t.addSection("BMP280");

  if (!bmpOk)
  {
    t.addRow(
        "Status",
        "ERROR");
  }
  else
  {
    Reading temp{};
    Reading press{};

    if (process(
            bmpTemperatureFilter,
            bmpTemperatureCalibration,
            bmpTemperatureValidator,
            bmpTemp,
            temp))
    {
      addTemperatureRows(
          t,
          temp);
    }
    else
    {
      t.addRow(
          "Temperature",
          "INVALID");
    }

    if (process(
            pressureFilter,
            pressureCalibration,
            pressureValidator,
            static_cast<int32_t>(
                pressure),
            press))
    {
      // Pressure is stored in Pa.
      // numFixed(..., 2, "hPa")
      // displays Pa / 100 as hPa.

      t.addRow(
          "Pressure raw",
          t.numFixed(
              press.raw,
              2,
              "hPa"));

      t.addRow(
          "Pressure filtered",
          t.numFixed(
              press.filtered,
              2,
              "hPa"));

      t.addRow(
          "Pressure calibrated",
          t.numFixed(
              press.calibrated,
              2,
              "hPa"));

      t.addRow(
          "Approx altitude",
          t.numFixed(
              bmp280.calculateAltitude(
                  static_cast<uint32_t>(
                      press.calibrated)),
              2,
              "m"));
    }
    else
    {
      t.addRow(
          "Pressure",
          "INVALID");
    }
  }

  t.print();
}

// ============================================================
// Automatic calibration
// ============================================================
//
// CAL does:
//
// 1. Read sensors
// 2. Validate values
// 3. Reset filters
// 4. Put current values through MovingAverage
// 5. Calculate zero-point offsets
// 6. Apply offsets
// 7. Save calibration to W25Q128
//
// IMPORTANT:
// This is ZERO calibration.
// Current sensor values become the zero/reference point.
//
// It is NOT absolute calibration against an external reference.
// ============================================================

bool Command::calibrateSensors()
{
  Serial2.println();
  Serial2.println(
      "Starting calibration...");

  int16_t ahtTemp = 0;
  uint16_t humidity = 0;

  int16_t bmpTemp = 0;
  uint32_t pressure = 0;

  // ----------------------------------------------------------
  // Read AHT20
  // ----------------------------------------------------------

  const bool ahtOk =
      aht20.read(
          ahtTemp,
          humidity);

  // ----------------------------------------------------------
  // Read BMP280
  // ----------------------------------------------------------

  const bool bmpOk =
      bmp280.read(
          bmpTemp,
          pressure);

  if (!ahtOk)
  {
    Serial2.println(
        "CAL: AHT20 read ERROR");

    return false;
  }

  if (!bmpOk)
  {
    Serial2.println(
        "CAL: BMP280 read ERROR");

    return false;
  }

  // ----------------------------------------------------------
  // Validate AHT temperature
  // ----------------------------------------------------------

  const ValidationResult ahtResult =
      ahtTemperatureValidator.check(
          ahtTemp);

  if (!Validator::isValid(
          ahtResult))
  {
    Serial2.println(
        "CAL: AHT temperature INVALID");

    return false;
  }

  // ----------------------------------------------------------
  // Validate BMP temperature
  // ----------------------------------------------------------

  const ValidationResult bmpResult =
      bmpTemperatureValidator.check(
          bmpTemp);

  if (!Validator::isValid(
          bmpResult))
  {
    Serial2.println(
        "CAL: BMP temperature INVALID");

    return false;
  }

  // ----------------------------------------------------------
  // Validate pressure
  // ----------------------------------------------------------

  const int32_t pressureValue =
      static_cast<int32_t>(
          pressure);

  const ValidationResult pressureResult =
      pressureValidator.check(
          pressureValue);

  if (!Validator::isValid(
          pressureResult))
  {
    Serial2.println(
        "CAL: pressure INVALID");

    return false;
  }

  // ----------------------------------------------------------
  // Reset filters
  // ----------------------------------------------------------

  ahtTemperatureFilter.reset();
  bmpTemperatureFilter.reset();
  pressureFilter.reset();

  // ----------------------------------------------------------
  // Filter current values
  // ----------------------------------------------------------

  const int32_t ahtFiltered =
      ahtTemperatureFilter.update(
          ahtTemp);

  const int32_t bmpFiltered =
      bmpTemperatureFilter.update(
          bmpTemp);

  const int32_t pressureFiltered =
      pressureFilter.update(
          pressureValue);

  // ----------------------------------------------------------
  // Zero calibration
  //
  // current value + offset = 0
  // ----------------------------------------------------------

  const int32_t ahtOffset =
      -ahtFiltered;

  const int32_t bmpOffset =
      -bmpFiltered;

  const int32_t pressureOffset =
      -pressureFiltered;

  // ----------------------------------------------------------
  // Apply calibration
  // ----------------------------------------------------------

  ahtTemperatureCalibration.setOffset(
      ahtOffset);

  bmpTemperatureCalibration.setOffset(
      bmpOffset);

  pressureCalibration.setOffset(
      pressureOffset);

  // ----------------------------------------------------------
  // Print result
  // ----------------------------------------------------------

  Table t(Serial2);

  t.setTitle(
       "Calibration result")
      .setAlign(
          1,
          Table::Align::Right);

  t.addSection("AHT20");

  t.addRow(
      "Raw",
      t.numFixed(
          ahtTemp,
          2,
          "°C"));

  t.addRow(
      "Filtered",
      t.numFixed(
          ahtFiltered,
          2,
          "°C"));

  t.addRow(
      "Offset",
      t.numFixed(
          ahtOffset,
          2,
          "°C"));

  t.addSection("BMP280");

  t.addRow(
      "Temperature raw",
      t.numFixed(
          bmpTemp,
          2,
          "°C"));

  t.addRow(
      "Temperature filtered",
      t.numFixed(
          bmpFiltered,
          2,
          "°C"));

  t.addRow(
      "Temperature offset",
      t.numFixed(
          bmpOffset,
          2,
          "°C"));

  t.addRow(
      "Pressure raw",
      t.numFixed(
          pressureValue,
          2,
          "hPa"));

  t.addRow(
      "Pressure filtered",
      t.numFixed(
          pressureFiltered,
          2,
          "hPa"));

  t.addRow(
      "Pressure offset",
      t.numFixed(
          pressureOffset,
          2,
          "Pa"));

  t.print();

  // ----------------------------------------------------------
  // Save immediately to W25Q128
  // ----------------------------------------------------------

  if (!saveCalibration())
  {
    Serial2.println(
        "CAL: FLASH SAVE ERROR");

    return false;
  }

  Serial2.println(
      "CAL: saved to FLASH");

  return true;
}

// ============================================================
// Load calibration
// ============================================================

void Command::setCalibration(
    const CalibrationData &cal)
{
  ahtTemperatureCalibration.setOffset(
      cal.ahtTemperatureOffset);

  bmpTemperatureCalibration.setOffset(
      cal.bmpTemperatureOffset);

  pressureCalibration.setOffset(
      cal.pressureOffset);
}

// ============================================================
// Print current calibration
// ============================================================

void Command::printCalibration()
{
  Serial2.println();
  Serial2.println(
      "Calibration:");

  Serial2.print(
      "AHT temperature offset: ");

  Serial2.println(
      ahtTemperatureCalibration.getOffset());

  Serial2.print(
      "BMP temperature offset: ");

  Serial2.println(
      bmpTemperatureCalibration.getOffset());

  Serial2.print(
      "Pressure offset: ");

  Serial2.println(
      pressureCalibration.getOffset());

  Serial2.println();
}

// ============================================================
// Save calibration
// ============================================================

bool Command::saveCalibration()
{
  const CalibrationData calibration =
      getCalibration();

  return calibrationStorage.save(
      calibration);
}

// ============================================================
// Get calibration
// ============================================================

CalibrationData Command::getCalibration() const
{
  CalibrationData cal{};

  cal.ahtTemperatureOffset =
      ahtTemperatureCalibration.getOffset();

  cal.bmpTemperatureOffset =
      bmpTemperatureCalibration.getOffset();

  cal.pressureOffset =
      pressureCalibration.getOffset();

  return cal;
}
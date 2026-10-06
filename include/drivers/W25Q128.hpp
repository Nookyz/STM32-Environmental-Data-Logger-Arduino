#pragma once

#include <Arduino.h>
#include <SPI.h>

// W25Q128 (16 MB) SPI NOR flash driver.
// Rules of NOR flash: a sector (4 KB) must be erased before writing,
// writes can only turn bits from 1 to 0.
class W25Q128
{
public:
  static constexpr uint32_t CAPACITY_BYTES = 16UL * 1024UL * 1024UL;
  static constexpr uint16_t PAGE_SIZE = 256;
  static constexpr uint16_t SECTOR_SIZE = 4096;

  W25Q128(SPIClass &spi, uint8_t csPin);

  // Initializes SPI and checks the JEDEC ID
  bool begin();

  bool read(uint32_t address, uint8_t *data, uint16_t length);

  // Writes across page boundaries; the target area must be erased
  bool write(uint32_t address, const uint8_t *data, uint16_t length);

  // Erases the 4 KB sector that contains the address
  bool eraseSector(uint32_t address);

private:
  static constexpr uint32_t SPI_CLOCK_HZ = 21000000UL; // 84 MHz / 4

  static constexpr uint8_t CMD_WRITE_ENABLE = 0x06;
  static constexpr uint8_t CMD_READ_STATUS1 = 0x05;
  static constexpr uint8_t CMD_READ_DATA = 0x03;
  static constexpr uint8_t CMD_PAGE_PROGRAM = 0x02;
  static constexpr uint8_t CMD_SECTOR_ERASE = 0x20;
  static constexpr uint8_t CMD_RELEASE_POWER_DOWN = 0xAB;
  static constexpr uint8_t CMD_JEDEC_ID = 0x9F;

  static constexpr uint8_t STATUS_BUSY = 0x01;

  static constexpr uint8_t MANUFACTURER_WINBOND = 0xEF;
  static constexpr uint8_t CAPACITY_ID_128M = 0x18;

  static constexpr uint32_t PAGE_PROGRAM_TIMEOUT_MS = 10;
  static constexpr uint32_t SECTOR_ERASE_TIMEOUT_MS = 500;

  SPIClass &spi;
  uint8_t csPin;
  SPISettings settings;

  void select();
  void deselect();
  void sendCommandWithAddress(uint8_t command, uint32_t address);
  void writeEnable();
  bool waitReady(uint32_t timeoutMs);
  bool writePage(uint32_t address, const uint8_t *data, uint16_t length);
};
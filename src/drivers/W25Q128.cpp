#include "drivers/W25Q128.hpp"

W25Q128::W25Q128(SPIClass &spi, uint8_t csPin)
    : spi(spi),
      csPin(csPin),
      settings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0)
{
}

void W25Q128::select()
{
  spi.beginTransaction(settings);
  digitalWrite(csPin, LOW);
}

void W25Q128::deselect()
{
  digitalWrite(csPin, HIGH);
  spi.endTransaction();
}

void W25Q128::sendCommandWithAddress(uint8_t command, uint32_t address)
{
  spi.transfer(command);
  spi.transfer(static_cast<uint8_t>(address >> 16));
  spi.transfer(static_cast<uint8_t>(address >> 8));
  spi.transfer(static_cast<uint8_t>(address));
}

bool W25Q128::begin()
{
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);

  spi.begin();

  // Wake the chip up in case it was left in power-down mode
  select();
  spi.transfer(CMD_RELEASE_POWER_DOWN);
  deselect();
  delayMicroseconds(5);

  select();
  spi.transfer(CMD_JEDEC_ID);
  const uint8_t manufacturer = spi.transfer(0);
  spi.transfer(0); // memory type, differs between JV/FV/IM variants
  const uint8_t capacity = spi.transfer(0);
  deselect();

  return manufacturer == MANUFACTURER_WINBOND &&
         capacity == CAPACITY_ID_128M;
}

void W25Q128::writeEnable()
{
  select();
  spi.transfer(CMD_WRITE_ENABLE);
  deselect();
}

bool W25Q128::waitReady(uint32_t timeoutMs)
{
  const uint32_t start = millis();

  while (millis() - start < timeoutMs)
  {
    select();
    spi.transfer(CMD_READ_STATUS1);
    const uint8_t status = spi.transfer(0);
    deselect();

    if ((status & STATUS_BUSY) == 0)
    {
      return true;
    }
  }

  return false;
}

bool W25Q128::read(uint32_t address, uint8_t *data, uint16_t length)
{
  if (address + length > CAPACITY_BYTES)
  {
    return false;
  }

  select();
  sendCommandWithAddress(CMD_READ_DATA, address);

  for (uint16_t i = 0; i < length; ++i)
  {
    data[i] = spi.transfer(0);
  }

  deselect();
  return true;
}

bool W25Q128::writePage(uint32_t address, const uint8_t *data,
                        uint16_t length)
{
  writeEnable();

  select();
  sendCommandWithAddress(CMD_PAGE_PROGRAM, address);

  for (uint16_t i = 0; i < length; ++i)
  {
    spi.transfer(data[i]);
  }

  deselect();

  return waitReady(PAGE_PROGRAM_TIMEOUT_MS);
}

bool W25Q128::write(uint32_t address, const uint8_t *data, uint16_t length)
{
  if (address + length > CAPACITY_BYTES)
  {
    return false;
  }

  while (length > 0)
  {
    // A page program must not cross a 256-byte page boundary
    const uint16_t room = PAGE_SIZE - (address % PAGE_SIZE);
    const uint16_t chunk = (length < room) ? length : room;

    if (!writePage(address, data, chunk))
    {
      return false;
    }

    address += chunk;
    data += chunk;
    length -= chunk;
  }

  return true;
}

bool W25Q128::eraseSector(uint32_t address)
{
  if (address >= CAPACITY_BYTES)
  {
    return false;
  }

  writeEnable();

  select();
  sendCommandWithAddress(CMD_SECTOR_ERASE, address & ~(SECTOR_SIZE - 1UL));
  deselect();

  return waitReady(SECTOR_ERASE_TIMEOUT_MS);
}
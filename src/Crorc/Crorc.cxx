/// \file Crorc.cxx
/// \brief Implementation of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Crorc.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <boost/format.hpp>
#include <pda.h>
#include "Rorc.h"
#include "ExceptionInternal.h"
#include "RORC/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace Rorc {
namespace Crorc {
namespace Flash {

constexpr int MAX_WAIT = 1000000;
constexpr int DATA_STATUS = F_IFDSR;
constexpr int ADDRESS = F_IADR;
constexpr int READY = F_LRD;

unsigned readStatus(RegisterReadWriteInterface& channel, int sleept = 1)
{
  unsigned stat;

  channel.writeRegister(DATA_STATUS, 0x04000000);
  usleep(sleept);
  stat = channel.readRegister(ADDRESS);

  return (stat);
}

uint32_t init(RegisterReadWriteInterface& channel, int sleept = 10)
{
  // Clear Status register
  channel.writeRegister(DATA_STATUS, 0x03000050);
  usleep(10*sleept);

  // Set ASYNCH mode (Configuration Register 0xBDDF)
  channel.writeRegister(DATA_STATUS, 0x0100bddf);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, 0x03000060);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, 0x03000003);
  usleep(sleept);

  // Read Status register
  uint32_t address = 0x01000000;
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, 0x03000070);
  usleep(sleept);

  uint32_t stat = readStatus(channel);
  return stat;
}

int checkStatus(RegisterReadWriteInterface& channel, int timeout)
{
  unsigned stat;
  int i = 0;

  while (1)
  {
    stat = readStatus(channel);
    if (stat == 0x80) {
      break;
    }
    if (timeout && (++i >= timeout)) {
      return (RORC_TIMEOUT);
    }
    usleep(100);
  }

  return (RORC_STATUS_OK);
}

void unlockBlock(RegisterReadWriteInterface& channel, uint32_t address, int sleept)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, 0x03000060);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, 0x030000d0);
  usleep(sleept);

  int ret = checkStatus(channel, MAX_WAIT);
  if (ret) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Couldn't unlock flash block")
        << ErrorInfo::Address(address));
  }
}

void eraseBlock(RegisterReadWriteInterface& channel, uint32_t address, int sleept)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, 0x03000020);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, 0x030000d0);
  usleep(sleept);

  int ret = checkStatus(channel, MAX_WAIT);
  if (ret) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Couldn't erase flash block")
        << ErrorInfo::Address(address));
  }
}

int writeWord(RegisterReadWriteInterface& channel, uint32_t address, int value, int sleept)
{
  int ret;

  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, 0x03000040);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, value);
  usleep(sleept);
  ret = checkStatus(channel, MAX_WAIT);

  return (ret);
}

int readWord(RegisterReadWriteInterface& channel, uint32_t address, uint8_t *data, int sleept)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, 0x030000ff);
  usleep(sleept);
  uint32_t stat = readStatus(channel, sleept);
  *data = (stat & 0xFF00) >> 8;
  *(data+1) = stat & 0xFF;
  return (RORC_STATUS_OK);
}

void readRange(RegisterReadWriteInterface& channel, int addressFlash, int wordNumber, std::ostream& out) {
  for (int i = addressFlash; i < (addressFlash + wordNumber); ++i) {
    uint32_t address = i;
    address = 0x01000000 | address;

    channel.writeRegister(DATA_STATUS, address);
    usleep(50);

    channel.writeRegister(DATA_STATUS, 0x030000ff);
    usleep(50);

    channel.writeRegister(DATA_STATUS, 0x04000000);
    usleep(50);

    uint32_t status = channel.readRegister(ADDRESS);
    uint32_t status2 = channel.readRegister(READY);

    out << boost::format("%5u  %d\n") % status % status2;
  }
}

void wait(RegisterReadWriteInterface& channel) {
  int status = 0;
  while (status == 0) {
    status = channel.readRegister(READY);
  }
  usleep(1);
}

} // namepsace Flash

void readFlashRange(RegisterReadWriteInterface& channel, int addressFlash, int wordNumber, std::ostream& out)
{
  Flash::readRange(channel, addressFlash, wordNumber, out);
}

/// Based on "pdaCrorcFlashProgrammer.c"
/// I don't really understand what it does.
void programFlash(RegisterReadWriteInterface& channel, std::string dataFilePath, int addressFlash, std::ostream& out,
    const std::atomic<bool>* interrupt)
{
  using boost::format;

  struct InterruptedException : public std::exception {};

  auto checkInterrupt = [&]{
    if (interrupt != nullptr) {
      if (interrupt->load()) {
        throw InterruptedException();
      }
    };
  };

  auto writeWait = [&](int index, int value) {
    channel.writeRegister(index, value);
    Flash::wait(channel);
  };

  auto readWait = [&](int index) {
    uint32_t value = channel.readRegister(index);
    Flash::wait(channel);
    return value;
  };

  try {
    // Open file
    std::unique_ptr<FILE, void (*)(FILE*)> file(fopen(dataFilePath.c_str(), "r"), [](FILE* f) {fclose(f);});
    if (file.get() == nullptr) {
      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Failed to open file")
          << ErrorInfo::FileName(dataFilePath));
    }

    // Initiate flash: clear status register, set asynch mode, read status reg.
    out << "Init flash\n";
    uint64_t status = Flash::init(channel);
    out << format("Status = 0x%lX\n\n") % status;

    uint32_t address = 0x01000000;

    // (0x460000 is the last BA used by the CRORC firmware)
    out << "Unlocking and erasing blocks:\n";
    while (address <= 0x01460000) {
      checkInterrupt();

      out << format("\r    Block - 0x%X") % address << std::flush;
      Flash::unlockBlock(channel, address, 10);
      Flash::eraseBlock(channel, address, 10);

      // increase BA
      address = address + 0x010000;
    }

    // Write data
    out << "\nWriting:\n";

    int numberOfLinesRead = 0;
    int stop = 0;
    int maxCount = 0;
    address = 0x01000000;

    if (addressFlash != 0) {
      address = 0x01000000 | addressFlash;
    }

    while (stop != 1) {
      checkInterrupt();

      // set address WRITE IN THE FLASH
      writeWait(Flash::DATA_STATUS, address);

      // SET Buffer Program
      writeWait(Flash::DATA_STATUS, 0x030000E8);

      // READ STATUS REGISTER
      if (Flash::checkStatus(channel, Flash::MAX_WAIT)) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Couldn't set flash buffer mode")
            << ErrorInfo::Address(address));
      }

      // Write 32 words (31+1) (31 0x1f)
      writeWait(Flash::DATA_STATUS, 0x0300001F);

      // Read 32 words from file
      // Every word is on its own 'line' in the file
      for (int j = 0; j < 32; j++) {
        checkInterrupt();

        constexpr int BYTES = 10;
        char data[BYTES];

        if (fgets(data, BYTES, file.get()) == NULL) {
          stop = 1;
          break;
        }

        int hexValue = 0x03000000;
        int hex = std::atoi(data);
        hexValue += hex;

        writeWait(Flash::DATA_STATUS, address);
        writeWait(Flash::DATA_STATUS, hexValue);

        address++;
        numberOfLinesRead++;
        if (numberOfLinesRead % 1000 == 0) {
          constexpr float MAX_WORDS = 4616222.0;
          float perc = ((float) numberOfLinesRead / MAX_WORDS) * 100.0;
          out << format("\r    Progress - %1.1f%%") % perc << std::flush;
        }
      }

      writeWait(Flash::DATA_STATUS, 0x030000D0);
      writeWait(Flash::DATA_STATUS, 0x04000000);

      status = channel.readRegister(Flash::ADDRESS);
      while (status != 0x80) {
        checkInterrupt();

        writeWait(Flash::DATA_STATUS, 0x04000000);
        status = readWait(Flash::ADDRESS);

        maxCount++;
        if (maxCount == 5000000) {
          BOOST_THROW_EXCEPTION(Exception()
              << ErrorInfo::Message("Flash was stuck")
              << ErrorInfo::Address(address));
        }
      }

      if (Flash::checkStatus(channel, Flash::MAX_WAIT)) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Flash status was not correct after writing address")
            << ErrorInfo::Address(address));
      }
    }
    out << format("\nCompleted total %d words programmed\n") % numberOfLinesRead;
    // READ STATUS REG
    channel.writeRegister(Flash::DATA_STATUS, 0x03000070);
    usleep(1);

    if (Flash::checkStatus(channel, Flash::MAX_WAIT)) {
      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Couldn't read flash status")
          << ErrorInfo::Address(address));
    }
  } catch (const InterruptedException& e) {
    out << "Flash programming interrupted\n";
  }
}

// We include this C header that only getSerial() needs down here to avoid polluting the namespace for the rest
#include "c/rorc/rorc.h"

// TODO Clean up, C++ificate
int getSerial(uintptr_t barAddress)
{
  // Getting the serial number of the card from the flash. See rorcSerial() in rorc_lib.c.
  char data[RORC_SN_LENGTH + 1];
  memset(data, 'x', RORC_SN_LENGTH + 1);

  // Reading the FLASH.
  uint32_t flashAddr = FLASH_SN_ADDRESS;
  initFlash(barAddress, flashAddr, 10); // TODO check returned status code

  // Setting the address to the serial number's (SN) position. (Actually it's the position
  // one before the SN's, because we need even position and the SN is at odd position.
  flashAddr += (RORC_SN_POSITION - 1) / 2;
  data[RORC_SN_LENGTH] = '\0';
  // Retrieving information character by caracter from the HW ID string.
  for (int i = 0; i < RORC_SN_LENGTH; i += 2, flashAddr++) {
    readFlashWord(barAddress, flashAddr, &data[i], 10);
    if ((data[i] == '\0') || (data[i + 1] == '\0')) {
      break;
    }
  }

  // We started reading the serial number one position before, so we don't need
  // the first character.
  memmove(data, data+1, strlen(data));
  int serial = 0;
  serial = atoi(data);

  return serial;
}

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2


// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file Crorc.cxx
/// \brief Implementation of low level C-RORC functions
///
/// This scary looking file is a work in progress translation of the old C interface.
/// It contains functions that do the nitty-gritty low-level communication with the C-RORC.
/// Much of it is not fully understood.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Crorc.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include "Crorc/Constants.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

#include <boost/optional/optional_io.hpp>

using namespace std::chrono_literals;
using std::this_thread::sleep_for;
namespace b = boost;
namespace chrono = std::chrono;

namespace
{
int logi2(unsigned int number)
{
  int i;
  unsigned int mask = 0x80000000;

  for (i = 0; i < 32; i++) {
    if (number & mask) {
      break;
    }
    mask = mask >> 1;
  }

  return (31 - i);
}

int roundPowerOf2(int number)
{
  return (1 << logi2(number));
}

// Translations of old macros
auto ST_DEST = [](auto fw) { return ((unsigned short)((fw)&0xf)); };
auto mask = [](auto a, auto b) { return a & b; };
auto incr15 = [](auto a) { return (((a) + 1) & 0xf); };
} // Anonymous namespace

namespace o2
{
namespace roc
{
namespace Crorc
{
namespace Flash
{
namespace
{
constexpr int MAX_WAIT = 1000000;
constexpr int REGISTER_DATA_STATUS = Rorc::Flash::IFDSR;
constexpr int REGISTER_ADDRESS = Rorc::Flash::IADR;
constexpr int REGISTER_READY = Rorc::Flash::LRD;
constexpr int DDL_MAX_HW_ID = 64;
constexpr int SN_POS = 33;

// TODO figure out what these are/do
constexpr int MAGIC_VALUE_0 = 0x80;
constexpr int MAGIC_VALUE_3 = 0x0100bddf;
constexpr int MAGIC_VALUE_13 = 0x03000000;
constexpr int MAGIC_VALUE_5 = 0x03000003;
constexpr int MAGIC_VALUE_12 = 0x0300001f;
constexpr int MAGIC_VALUE_8 = 0x03000020;
constexpr int MAGIC_VALUE_9 = 0x03000040;
constexpr int MAGIC_VALUE_2 = 0x03000050;
constexpr int MAGIC_VALUE_4 = 0x03000060;
constexpr int MAGIC_VALUE_6 = 0x03000070;
constexpr int MAGIC_VALUE_7 = 0x030000d0;
constexpr int MAGIC_VALUE_11 = 0x030000e8;
constexpr int MAGIC_VALUE_10 = 0x030000ff;
constexpr int MAGIC_VALUE_1 = 0x04000000;

constexpr uint32_t ADDRESS_START = 0x01000000;
constexpr uint32_t ADDRESS_END = 0x01460000;
constexpr uint32_t BLOCK_SIZE = 0x010000;
constexpr size_t MAX_WORDS = 4616222;

/// Writes and sleeps
template <typename SleepTime>
void writeSleep(RegisterReadWriteInterface& bar0, int index, int value, SleepTime sleepTime)
{
  bar0.writeRegister(index, value);
  sleep_for(sleepTime);
}

/// Writes to F_IFDSR and sleeps
template <typename SleepTime = std::chrono::microseconds>
void writeStatusSleep(RegisterReadWriteInterface& bar0, int value, SleepTime sleepTime = 10us)
{
  writeSleep(bar0, REGISTER_DATA_STATUS, value, sleepTime);
}

unsigned readStatus(RegisterReadWriteInterface& bar0)
{
  writeStatusSleep(bar0, MAGIC_VALUE_1, 1us);
  return bar0.readRegister(REGISTER_ADDRESS);
}

uint32_t init(RegisterReadWriteInterface& bar0, uint32_t address)
{
  // Clear Status register
  writeStatusSleep(bar0, MAGIC_VALUE_2, 100us);
  // Set ASYNCH mode (Configuration Register 0xBDDF)
  writeStatusSleep(bar0, MAGIC_VALUE_3);
  writeStatusSleep(bar0, MAGIC_VALUE_4);
  writeStatusSleep(bar0, MAGIC_VALUE_5);
  // Read Status register
  writeStatusSleep(bar0, address, 10us);
  writeStatusSleep(bar0, MAGIC_VALUE_6);
  return readStatus(bar0);
}

void checkStatus(RegisterReadWriteInterface& channel)
{
  for (int i = 0; i < MAX_WAIT; ++i) {
    if (readStatus(channel) == MAGIC_VALUE_0) {
      return;
    }
    sleep_for(100us);
  }
  BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Bad flash status"));
}

void unlockBlock(RegisterReadWriteInterface& bar0, uint32_t address)
{
  //  writeStatusSequence(bar0, 10us, {MAGIC_VALUE_3, address, MAGIC_VALUE_4, MAGIC_VALUE_7});
  writeStatusSleep(bar0, MAGIC_VALUE_3);
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_4);
  writeStatusSleep(bar0, MAGIC_VALUE_7);
  checkStatus(bar0);
}

void eraseBlock(RegisterReadWriteInterface& bar0, uint32_t address)
{
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_8);
  writeStatusSleep(bar0, MAGIC_VALUE_7);
  checkStatus(bar0);
}

/// Currently unused, but we'll keep it as "documentation"
void writeWord(RegisterReadWriteInterface& bar0, uint32_t address, int value)
{
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_9);
  writeStatusSleep(bar0, value);
  checkStatus(bar0);
}

/// Reads a 16-bit flash word and writes it into the given buffer
void readWord(RegisterReadWriteInterface& bar0, uint32_t address, char* data)
{
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_10);

  uint32_t stat = readStatus(bar0);
  data[0] = (stat & 0xFF00) >> 8;
  data[1] = stat & 0xFF;
}

void readRange(RegisterReadWriteInterface& bar0, int addressFlash, int wordNumber, std::ostream& out)
{
  for (int i = addressFlash; i < (addressFlash + wordNumber); ++i) {
    uint32_t address = i;
    address = 0x01000000 | address;

    writeStatusSleep(bar0, address, 50us);
    writeStatusSleep(bar0, MAGIC_VALUE_10, 50us);
    writeStatusSleep(bar0, MAGIC_VALUE_1, 50us);

    uint32_t status = bar0.readRegister(REGISTER_ADDRESS);
    uint32_t status2 = bar0.readRegister(REGISTER_READY);

    out << b::format("%5u  %d\n") % status % status2;
  }
}

void wait(RegisterReadWriteInterface& channel)
{
  int status = 0;
  while (status == 0) {
    status = channel.readRegister(REGISTER_READY);
  }
  sleep_for(1us);
}
} // Anonymous namespace
} // namespace Flash

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
  struct InterruptedException : public std::exception {
  };

  auto checkInterrupt = [&] {
    if (interrupt != nullptr && interrupt->load(std::memory_order_relaxed)) {
      throw InterruptedException();
    }
  };

  auto writeStatusWait = [&](int value) {
    channel.writeRegister(Flash::REGISTER_DATA_STATUS, value);
    Flash::wait(channel);
  };

  auto readWait = [&](int index) {
    uint32_t value = channel.readRegister(index);
    Flash::wait(channel);
    return value;
  };

  // Open file
  std::ifstream ifstream{ dataFilePath };
  if (!ifstream.is_open()) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Failed to open file") << ErrorInfo::FileName(dataFilePath));
  }

  try {
    // Initiate flash: clear status register, set asynch mode, read status reg.
    out << "Initializing flash\n";
    uint64_t status = Flash::init(channel, Flash::ADDRESS_START);
    if (status != 0x80) {
      // 0x80 seems good, not sure what's bad
      out << format("    Status    0x%lX\n") % status;
    }

    uint32_t address = Flash::ADDRESS_START;

    // (0x460000 is the last BA used by the CRORC firmware)
    out << "Unlocking and erasing blocks\n";
    while (address <= Flash::ADDRESS_END) {
      checkInterrupt();

      out << format("\r  Block     0x%X") % address << std::flush;
      Flash::unlockBlock(channel, address);
      Flash::eraseBlock(channel, address);

      // increase BA
      address = address + Flash::BLOCK_SIZE;
    }

    // Write data
    out << "\nWriting\n";

    int numberOfLinesRead = 0;
    int stop = 0;
    int maxCount = 0;
    address = Flash::ADDRESS_START;

    if (addressFlash != 0) {
      address = Flash::ADDRESS_START | addressFlash;
    }

    while (stop != 1) {
      checkInterrupt();
      writeStatusWait(address);
      // Set buffer program
      writeStatusWait(Flash::MAGIC_VALUE_11);
      Flash::checkStatus(channel);
      // Write 32 words (31+1) (31 0x1f)
      writeStatusWait(Flash::MAGIC_VALUE_12);

      // Read 32 words from file
      // Every word is on its own 'line' in the file
      for (int j = 0; j < 32; j++) {
        checkInterrupt();

        int hex;
        if (!(ifstream >> hex)) {
          stop = 1;
          break;
        }

        writeStatusWait(address);
        writeStatusWait(Flash::MAGIC_VALUE_13 + hex);

        address++;
        numberOfLinesRead++;
        if (numberOfLinesRead % 1000 == 0) {
          float perc = ((float)numberOfLinesRead / Flash::MAX_WORDS) * 100.0;
          out << format("\r  Progress  %1.1f%%") % perc << std::flush;
        }
      }

      writeStatusWait(Flash::MAGIC_VALUE_7);
      writeStatusWait(Flash::MAGIC_VALUE_1);

      status = channel.readRegister(Flash::REGISTER_ADDRESS);
      while (status != 0x80) {
        checkInterrupt();

        writeStatusWait(Flash::MAGIC_VALUE_1);
        status = readWait(Flash::REGISTER_ADDRESS);

        maxCount++;
        if (maxCount == 5000000) {
          BOOST_THROW_EXCEPTION(Exception()
                                << ErrorInfo::Message("Flash was stuck")
                                << ErrorInfo::Address(address));
        }
      }
      Flash::checkStatus(channel);
    }
    out << format("\nCompleted programming %d words\n") % numberOfLinesRead;
    // READ STATUS REG
    channel.writeRegister(Flash::REGISTER_DATA_STATUS, Flash::MAGIC_VALUE_6);
    sleep_for(1us);
    Flash::checkStatus(channel);
  } catch (const InterruptedException& e) {
    out << "Flash programming interrupted\n";
  }
}

int Crorc::armDataGenerator(int dataPattern, uint32_t initEventNumber, uint32_t initDataWord,
                            int dataSize, int seed)
{
  int eventLen = dataSize / 4;

  if ((eventLen < 1) || (eventLen >= 0x00080000)) {
    BOOST_THROW_EXCEPTION(CrorcArmDataGeneratorException()
                          << ErrorInfo::Message("Failed to arm data generator; invalid event length")
                          << ErrorInfo::GeneratorEventLength(eventLen));
  }

  int roundedLen = eventLen;
  unsigned long blockLen = 0;
  if (seed) {
    /* round to the nearest lower power of two */
    roundedLen = roundPowerOf2(eventLen);
    blockLen = ((roundedLen - 1) << 4) | dataPattern;
    blockLen |= 0x80000000;
    write(Rorc::C_DG2, seed);
  } else {
    blockLen = ((eventLen - 1) << 4) | dataPattern;
    write(Rorc::C_DG2, initDataWord);
  }

  write(Rorc::C_DG1, blockLen);
  write(Rorc::C_DG3, initEventNumber);

  return roundedLen;
}

void Crorc::startDataGenerator(uint32_t maxLoop)
{
  uint32_t cycle;

  if (maxLoop) {
    cycle = (maxLoop - 1) & 0x7fffffff;
  } else {
    cycle = 0x80000000;
  }

  write(Rorc::C_DG4, cycle);
  write(Rorc::C_CSR, Rorc::CcsrCommand::START_DG);
}

void Crorc::stopDataGenerator()
{
  write(Rorc::C_CSR, Rorc::CcsrCommand::STOP_DG);
}

void Crorc::stopDataReceiver()
{
  if (read(Rorc::C_CSR) & Rorc::CcsrCommand::DATA_RX_ON_OFF) {
    write(Rorc::C_CSR, Rorc::CcsrCommand::DATA_RX_ON_OFF);
  }
}

/// Sends one command to the given link.
/// \param dest Command destination: 0 RORC, 1 DIU, 2 SIU, 4 FEE. If -1 then the full command is in the command field.
/// \param command Command code
/// \param transid Transaction ID
/// \param param Command parameter, or the full command if dest == -1
/// \param time If > 0 then test if command can be sent and wait as many cycles if necessary.
void Crorc::ddlSendCommand(int dest, uint32_t command, int transid, uint32_t param, long long int time)
{
  uint32_t com;
  int destination;

  if (dest == -1) {
    com = command;
    destination = com & 0xf;
  } else {
    destination = dest & 0xf;
    com = destination + ((command & 0xf) << 4) + ((transid & 0xf) << 8) + ((param & 0x7ffff) << 12);
  }

  if (destination > Ddl::Destination::DIU) {
    assertLinkUp();
  }

  long long int i;
  for (i = 0; i < time; i++) {
    if (checkCommandRegister() == 0) {
      break;
    }
  }

  if (time && (i == time)) {
    BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Timed out sending DDL command"));
  }

  putCommandRegister(com);
}

/// Checks whether status mail box or register is not empty in timeout
/// \param timeout  Number of check cycles
long long int Crorc::ddlWaitStatus(long long int timeout)
{
  for (int i = 0; i < timeout; ++i) {
    if (checkRxStatus()) {
      return i;
    }
  }
  BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Timed out waiting on DDL"));
}

/// Call ddlWaitStatus() before this routine if status timeout is needed
StWord Crorc::ddlReadStatus()
{
  StWord stw;
  stw.stw = read(Rorc::C_DSR);
  //  printf("ddlReadStatus: status = %08lx\n", stw.stw);
  return stw;
}

StWord Crorc::ddlReadDiu(int transid, long long int time)
{
  /// Prepare and send DDL command
  int destination = Ddl::Destination::DIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time);
  ddlWaitStatus(time);
  StWord stw = ddlReadStatus();
  if (stw.part.code != Rorc::IFSTW || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Unexpected DIU STW (not IFSTW)")
                  << ErrorInfo::StwExpected((b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination).str())
                  << ErrorInfo::StwReceived((b::format("0x%08lx") % stw.stw).str()));
  }
  StWord ret = stw;

  stw = ddlReadCTSTW(transid, destination, time); // XXX Not sure why we do this...
  return ret;
}

StWord Crorc::ddlReadCTSTW(int transid, int destination, long long int time)
{
  ddlWaitStatus(time);
  StWord stw = ddlReadStatus();
  if ((stw.part.code != Rorc::CTSTW && stw.part.code != Rorc::ILCMD && stw.part.code != Rorc::CTSTW_TO) || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Unexpected STW (not CTSTW)")
                  << ErrorInfo::StwExpected((b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination).str())
                  << ErrorInfo::StwReceived((b::format("0x%08lx") % stw.stw).str()));
  }
  return stw;
}

//TODO: Transid could be set automatically here
StWord Crorc::ddlReadSiu(int transid, long long int time) //TODO: This to be kept like this -> clean wherever possible
{
  // prepare and send DDL command
  int destination = Ddl::Destination::SIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time); //TODO: A bar write

  // read and check the answer
  ddlWaitStatus(time);          //TODO: Keep reading to check the status
  StWord stw = ddlReadStatus(); //TODO: Read once
  if (stw.part.code != Rorc::IFSTW || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Unexpected SIU STW (not IFSTW)")
                  << ErrorInfo::StwExpected((b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination).str())
                  << ErrorInfo::StwReceived((b::format("0x%08lx") % stw.stw).str()));
  }

  StWord ret = stw;
  stw = ddlReadStatus(); //TODO: Read twice
  if ((stw.part.code != Rorc::CTSTW && stw.part.code != Rorc::ILCMD && stw.part.code != Rorc::CTSTW_TO) || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Unexpected SIU STW (not CTSTW)")
                  << ErrorInfo::StwExpected((b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination).str())
                  << ErrorInfo::StwReceived((b::format("0x%08lx") % stw.stw).str()));
  }
  return ret;
}

/// Interpret DIU or SIU IFSTW to user readable messages
/// Unused, kept for now as "documentation"
std::vector<std::string> ddlInterpretIFSTW(uint32_t ifstw)
{
  using namespace Ddl;
  using Table = std::vector<std::pair<uint32_t, const char*>>;

  const int destination = ST_DEST(ifstw);
  const uint32_t status = ifstw & STMASK;
  std::vector<std::string> messages;

  auto add = [&](const char* message) { messages.push_back(message); };

  auto checkTable = [&](uint32_t status, const Table& table) {
    for (const auto& pair : table) {
      if (mask(pair.first, status)) {
        add(pair.second);
      }
    }
  };

  auto checkTableExclusive = [&](uint32_t status, const Table& table) {
    for (const auto& pair : table) {
      if (mask(pair.first, status)) {
        add(pair.second);
        break;
      }
    }
  };

  if (destination == Destination::DIU) {
    using namespace Diu;
    const Table errorTable{
      { LOSS_SYNC, "Loss of synchronization" },
      { TXOF, "Transmit data/status overflow" },
      { RES1, "Undefined DIU error" },
      { OSINFR, "Ordered set in frame" },
      { INVRX, "Invalid receive character in frame" },
      { CERR, "CRC error" },
      { RES2, "Undefined DIU error" },
      { DOUT, "Data out of frame" },
      { IFDL, "Illegal frame delimiter" },
      { LONG, "Too long frame" },
      { RXOF, "Received data/status overflow" },
      { FRERR, "Error in receive frame" }
    };
    const Table portTable{
      { PortState::TSTM, "DIU port in PRBS Test Mode state" },
      { PortState::POFF, "DIU port in Power Off state" },
      { PortState::LOS, "DIU port in Offline Loss of Synchr. state" },
      { PortState::NOSIG, "DIU port in Offline No Signal state" },
      { PortState::WAIT, "DIU port in Waiting for Power Off state" },
      { PortState::ONL, "DIU port in Online state" },
      { PortState::OFFL, "DIU port in Offline state" },
      { PortState::POR, "DIU port in Power On Reset state" }
    };

    if (mask(status, DIU_LOOP)) {
      add("DIU is set in loop-back mode");
    }
    if (mask(status, ERROR_BIT)) {
      checkTable(mask(status, DIUSTMASK), errorTable);
    }
    checkTableExclusive(status, portTable);
  } else {
    using namespace Siu;
    const Table okTable{
      { LBMOD, "SIU in Loopback Mode" },
      { OPTRAN, "One FEE transaction is open" }
    };
    const Table errorTable{
      { LONGE, "Too long event or read data block" },
      { IFEDS, "Illegal FEE data/status" },
      { TXOF, "Transmit FIFO overflow" },
      { IWDAT, "Illegal write data word" },
      { OSINFR, "Ordered set in frame" },
      { INVRX, "Invalid character in receive frame" },
      { CERR, "CRC error" },
      { DJLERR, "DTCC or JTCC error" },
      { DOUT, "Data out of receive frame" },
      { IFDL, "Illegal frame delimiter" },
      { LONG, "Too long receive frame" },
      { RXOF, "Receive FIFO overflow" },
      { FRERR, "Error in receive frame" },
      { LPERR, "Link protocol error" }
    };
    const Table portTable{
      { PortState::RESERV, "SIU port in undefined state" },
      { PortState::POFF, "SIU port in Power Off state" },
      { PortState::LOS, "SIU port in Offline Loss of Synchr. state" },
      { PortState::NOSIG, "SIU port in Offline No Signal state" },
      { PortState::WAIT, "SIU port in Waiting for Power Off state" },
      { PortState::ONL, "SIU port in Online state" },
      { PortState::OFFL, "SIU port in Offline state" },
      { PortState::POR, "SIU port in Power On Reset state" }
    };

    if (mask(status, ERROR_BIT)) {
      checkTable(status, errorTable);
    } else {
      checkTable(status, okTable);
    }
    checkTableExclusive(mask(status, SIUSTMASK), portTable);
  }

  return messages;
}

/// Tries to reset the SIU.
/// \param cycle Number of status checks
/// \param time Number of cycles to wait for command sending and replies
void Crorc::ddlResetSiu(int cycle, long long int time)
{
  ddlSendCommand(Ddl::Destination::DIU, Ddl::SRST, 0, 0, time);
  ddlWaitStatus(time);
  ddlReadStatus();

  int transid = 0xf;

  std::vector<std::string> statusStringsToReturn;
  std::vector<std::string> statusStrings;

  for (int i = 0; i < cycle; i++) {
    try {
      sleep_for(10ms);

      transid = incr15(transid);
      StWord stword = ddlReadDiu(transid, time);
      stword.stw &= Ddl::STMASK;

      if (stword.stw & Diu::ERROR_BIT) {
        statusStrings = ddlInterpretIfstw(stword.stw);
        statusStringsToReturn.insert(statusStringsToReturn.end(), statusStrings.begin(), statusStrings.end());
      } else if (mask(stword.stw, Siu::OPTRAN)) {
        statusStrings = ddlInterpretIfstw(stword.stw);
        statusStringsToReturn.insert(statusStringsToReturn.end(), statusStrings.begin(), statusStrings.end());
      }

      transid = incr15(transid);
      stword = ddlReadSiu(transid, time);
      stword.stw &= Ddl::STMASK;

      if (stword.stw & Diu::ERROR_BIT) {
        statusStrings = ddlInterpretIfstw(stword.stw);
        statusStringsToReturn.insert(statusStringsToReturn.end(), statusStrings.begin(), statusStrings.end());
      }

      return;
    } catch (const Exception& e) {
    }
  }

  // Prepare verbose error message
  std::stringstream ss;
  ss << "Failed to reset SIU" << std::endl;
  for (auto const& string : statusStringsToReturn) {
    ss << string << std::endl;
  }
  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(ss.str()));
}

/// Sends a reset command
void Crorc::resetCommand(int option, const DiuConfig& diuConfig)
{
  uint32_t command = 0;
  long long int longret, timeout;

  timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;

  if (option & Rorc::Reset::DIU) {
    command |= Rorc::CcsrCommand::RESET_DIU;
  }
  if (command) {
    write(Rorc::C_CSR, (uint32_t)command);
  }
  if (option & Rorc::Reset::SIU) {
    putCommandRegister(Rorc::DcrCommand::RESET_SIU);
    longret = ddlWaitStatus(timeout);
    if (longret < timeout)
      ddlReadStatus();
  }
  if (!option || (option & Rorc::Reset::RORC)) {
    write(Rorc::RCSR, Rorc::RcsrCommand::RESET_CHAN); //channel reset
  }
}

void Crorc::armDdl(/*int resetMask, const DiuConfig& diuConfig*/)
{
  //auto reset = [&](uint32_t command) { resetCommand(command, diuConfig); };

  /*  if (resetMask & Rorc::Reset::FEE) { //TODO: Remove
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Command not allowed"));
  }
  if (resetMask & Rorc::Reset::SIU) { //TODO: Remove
    ddlResetSiu(3, Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec);
  }*/
  /* SHOULD BE ONLY THIS u*/
  if (true) { //TODO: Test only with this
    //reset(Rorc::Reset::RORC);
    //reset(Rorc::Reset::SIU); //TODO: This doesn't happen now

    //  write(0x0, 0x1); //CRORC device
    write(0x0, 0x2);       //CRORC channel
    write(0x18 / 4, 0xf1); //SIU
    //sleep_for(100ms);
    write(0x18 / 4, 0xf1); //SIU
    write(0x0, 0x2);       //CRORC channel
    // write(0x0, 0x1); //CRORC device
    sleep_for(100ms); //TODO: Make this into a timeout
    //reset(Rorc::Reset::SIU);
    //reset(Rorc::Reset::RORC);
    assertLinkUp();
  }

  /*  if (resetMask & Rorc::Reset::LINK_UP) {
    reset(Rorc::Reset::RORC);
    reset(Rorc::Reset::DIU);
    reset(Rorc::Reset::SIU);
    sleep_for(100ms);
    assertLinkUp();

    reset(Rorc::Reset::SIU);
    reset(Rorc::Reset::DIU);
    reset(Rorc::Reset::RORC);
    sleep_for(100ms);
    assertLinkUp();
  }
  if (resetMask & Rorc::Reset::DIU) {
    reset(Rorc::Reset::DIU);
  }
  if (resetMask & Rorc::Reset::FF) {
    reset(Rorc::Reset::FF);
  }
  if (resetMask & Rorc::Reset::RORC) {
    reset(Rorc::Reset::RORC);
  }*/
}

auto Crorc::initDiuVersion() -> DiuConfig
{
  int maxLoop = 1000;
  auto start = chrono::steady_clock::now();
  for (int i = 0; i < maxLoop; i++) {
    (void)checkRxStatus();
  };
  auto end = chrono::steady_clock::now();

  DiuConfig diuConfig;
  diuConfig.pciLoopPerUsec = double(maxLoop) / chrono::duration<double, std::micro>(end - start).count();
  return diuConfig;
}

bool Crorc::isLinkUp()
{
  return !((read(Rorc::C_CSR) & Rorc::CcsrStatus::LINK_DOWN));
}

void Crorc::assertLinkUp()
{
  if (!isLinkUp()) {
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message("Link was not up"));
  }
}

void Crorc::siuCommand(int command)
{
  ddlReadSiu(command, Ddl::RESPONSE_TIME);
}

void Crorc::diuCommand(int command)
{
  ddlReadDiu(command, Ddl::RESPONSE_TIME);
}

void Crorc::startDataReceiver(uintptr_t readyFifoBusAddress)
{
  write(Rorc::C_RRBAR, (readyFifoBusAddress & 0xffffffff));
  if (sizeof(uintptr_t) > 4) {
    write(Rorc::C_RRBX, (readyFifoBusAddress >> 32));
  } else {
    write(Rorc::C_RRBX, 0x0);
  }
  if (!(read(Rorc::C_CSR) & Rorc::CcsrCommand::DATA_RX_ON_OFF)) {
    write(Rorc::C_CSR, Rorc::CcsrCommand::DATA_RX_ON_OFF);
  }
}

StWord Crorc::ddlSetSiuLoopBack(const DiuConfig& diuConfig)
{
  long long int timeout = diuConfig.pciLoopPerUsec * Ddl::RESPONSE_TIME;

  // Check SIU fw version
  ddlSendCommand(Ddl::Destination::SIU, Ddl::IFLOOP, 0, 0, timeout);
  ddlWaitStatus(timeout);

  StWord stword = ddlReadStatus();
  if (stword.part.code == Rorc::ILCMD) {
    // Illegal command => old version => send TSTMODE for loopback
    ddlSendCommand(Ddl::Destination::SIU, Ddl::TSTMODE, 0, 0, timeout);
    ddlWaitStatus(timeout);
  }

  if (stword.part.code != Rorc::CTSTW) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Error setting SIU loopback"));
  }

  // SIU loopback command accepted => check SIU loopback status
  stword = ddlReadSiu(0, timeout);
  if (stword.stw & Siu::LBMOD) {
    return stword; // SIU loopback set
  }

  // SIU loopback not set => set it
  ddlSendCommand(Ddl::Destination::SIU, Ddl::IFLOOP, 0, 0, timeout);

  long long ret;
  ret = ddlWaitStatus(timeout);
  if (ret >= timeout)
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Timeout waiting for DDL status when setting\
          SIU loopback"));
  return ddlReadStatus();
}

StWord Crorc::ddlSetDiuLoopBack(const DiuConfig& diuConfig)
{
  long long int timeout = diuConfig.pciLoopPerUsec * Ddl::RESPONSE_TIME;

  ddlSendCommand(Ddl::Destination::DIU, Ddl::IFLOOP, 0, 0, timeout);
  ddlWaitStatus(timeout);

  return ddlReadStatus();
}

void Crorc::setSiuLoopback(const DiuConfig& diuConfig)
{
  ddlSetSiuLoopBack(diuConfig);
}

void Crorc::setDiuLoopback(const DiuConfig& diuConfig)
{
  ddlSetDiuLoopBack(diuConfig);
}

void Crorc::startTrigger(const DiuConfig& diuConfig, uint32_t command)
//void Crorc::startTrigger(uint32_t command)
{
  if ((command != Fee::RDYRX) && (command != Fee::STBRD)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Trigger can only be started with RDYRX or STBRD."));
  }
  uint64_t timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;
  ddlSendCommand(Ddl::Destination::FEE, command, 0, 0, timeout);
  //ddlSendCommand(Ddl::Destination::FEE, command, 0, 0, 0);
  ddlWaitStatus(timeout);
  //ddlWaitStatus(0);
  ddlReadStatus();
}

void Crorc::stopTrigger(const DiuConfig& diuConfig)
{
  uint64_t timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;

  auto rorcStopTrigger = [&] {
    ddlSendCommand(Ddl::Destination::FEE, Fee::EOBTR, 0, 0, timeout);
    ddlWaitStatus(timeout);
    ddlReadStatus();
  };

  // Try stopping once
  try {
    rorcStopTrigger();
  } catch (const Exception& e) {
    std::cout << "STOP TRIGGER DDL TIME-OUT" << std::endl;
  }

  // Try stopping twice
  try {
    rorcStopTrigger();
  } catch (const Exception& e) {
    std::cout << "STOP TRIGGER DDL TIME-OUT" << std::endl;
  }
}

void Crorc::setLoopbackOn()
{
  if (!isLoopbackOn()) {
    toggleLoopback();
  }
}

void Crorc::setLoopbackOff()
{
  // Switches off both loopback and stop_on_error
  if (isLoopbackOn()) {
    toggleLoopback();
  }
}

/// Not sure
bool Crorc::isLoopbackOn()
{
  return (read(Rorc::C_CSR) & Rorc::CcsrCommand::LOOPB_ON_OFF) == 0 ? false : true;
}

/// Not sure
void Crorc::toggleLoopback()
{
  write(Rorc::C_CSR, Rorc::CcsrCommand::LOOPB_ON_OFF);
}

uint32_t Crorc::checkCommandRegister()
{
  return read(Rorc::C_CSR) & Rorc::CcsrStatus::CMD_NOT_EMPTY;
}

void Crorc::putCommandRegister(uint32_t command)
{
  write(Rorc::C_DCR, command);
}

uint32_t Crorc::checkRxStatus()
{
  return read(Rorc::C_CSR) & Rorc::CcsrStatus::RXSTAT_NOT_EMPTY;
}

uint32_t Crorc::checkRxData()
{
  return read(Rorc::C_CSR) & Rorc::CcsrStatus::RXDAT_NOT_EMPTY;
}

void Crorc::pushRxFreeFifo(uintptr_t blockAddress, uint32_t blockLength, uint32_t readyFifoIndex)
{
  write(Rorc::C_RAFX, arch64() ? (blockAddress >> 32) : 0x0);
  write(Rorc::C_RAFH, blockAddress & 0xffffffff);
  write(Rorc::C_RAFL, (blockLength << 8) | readyFifoIndex);
}

void Crorc::initReadoutTriggered(RegisterReadWriteInterface&)
{
  BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}

boost::optional<int32_t> getSerial(RegisterReadWriteInterface& bar0)
{
  // Reading the FLASH.
  uint32_t address = Rorc::Serial::FLASH_ADDRESS;
  Flash::init(bar0, address);

  // Setting the address to the serial number's (SN) position. (Actually it's the position
  // one before the SN's, because we need even position and the SN is at odd position.
  std::array<char, Rorc::Serial::LENGTH + 1> data{ '\0' };
  address += (Rorc::Serial::POSITION - 1) / 2;
  for (int i = 0; i < Rorc::Serial::LENGTH; i += 2, address++) {
    Flash::readWord(bar0, address, &data[i]);
    if ((data[i] == '\0') || (data[i + 1] == '\0')) {
      break;
    }
  }

  // We don't use the first character for the conversion, since we started reading one byte before the serial number
  // location in the flash
  uint32_t serial = 0;
  if (!boost::conversion::try_lexical_convert(data.data() + 1, Rorc::Serial::LENGTH, serial)) {
    return {};
  }

  if (serial == 0xFfffFfff) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("C-RORC reported invalid serial number 0xffffffff, "
                                                            "a fatal error may have occurred"));
  }

  return { int32_t(serial) };
}

void setSerial(RegisterReadWriteInterface& bar0, int serial)
{

  uint32_t address = Rorc::Serial::FLASH_ADDRESS;
  Flash::init(bar0, address);
  Flash::unlockBlock(bar0, address);
  Flash::eraseBlock(bar0, address);

  // Prepare the data string
  // It needs to be DDL_MAX_HW_ID long and
  // initialized with ' '
  char serialCString[Flash::DDL_MAX_HW_ID + 1];
  serialCString[0] = '\0';

  for (int i = 0; i < Flash::DDL_MAX_HW_ID - 1; i++) {
    serialCString[i] = ' ';
  }

  // "S/N: vwxyz" needs to start at SN_POS
  // followed by a ' '
  sprintf(&serialCString[Flash::SN_POS - 5], "S/N: %05d", serial);
  serialCString[Flash::SN_POS + 5] = ' ';
  serialCString[Flash::DDL_MAX_HW_ID - 1] = '\0';

  // Write the data to the flash
  uint32_t hexValue = 0x0;
  for (int i = 0; i < Flash::DDL_MAX_HW_ID; i += 2, address++) {
    hexValue = Flash::MAGIC_VALUE_13 +
               (serialCString[i] << 8) + serialCString[i + 1];
    Flash::writeWord(bar0, address, hexValue);
  }
}

uint8_t Crorc::ddlReadHw(int destination, int address, long long int time)
{
  // prepare and send DDL command
  int command = Ddl::RHWVER;
  int transid = 0;
  int param = address;
  ddlSendCommand(destination, command, 0, param, time);

  // read and check the answer
  ddlWaitStatus(time);

  StWord stw = ddlReadStatus();
  if (stw.part.code != Ddl::HWSTW || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
      Exception() << ErrorInfo::Message("Not HWSTW!")
                  << ErrorInfo::StwExpected((b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination).str())
                  << ErrorInfo::StwReceived((b::format("0x%08lx") % stw.stw).str()));
  }

  uint8_t hw = (stw.stw >> 20) & 0xff;

  stw = ddlReadCTSTW(transid, destination, time);
  return hw;
}

std::string Crorc::ddlGetHwInfo(int destination, long long int time)
{
  uint8_t data[Ddl::MAX_HW_ID];
  data[0] = '\0';

  int i;
  for (i = 0; i < Ddl::MAX_HW_ID; i++) {
    data[i] = ddlReadHw(destination, i, time);
    if (data[i] == '\0') {
      break;
    }
  }
  data[i] = '\0';

  std::string hwInfo((const char*)data);
  return hwInfo;
}

uint32_t Crorc::ddlPrintStatus(int destination, int time)
{
  StWord status;

  if (destination == Ddl::Destination::SIU) {
    status = ddlReadSiu(0, time);
  } else if (destination == Ddl::Destination::DIU) {
    status = ddlReadDiu(0, time);
  } else {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("DDL Status destination invalid"));
  }

  return status.stw;
}

std::tuple<std::string, uint32_t> Crorc::siuStatus()
{
  DiuConfig diuConfig = initDiuVersion();
  //  resetCommand(Rorc::Reset::SIU, diuConfig);
  //  sleep_for(100ms);

  long long int time = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;

  std::string hwInfo = ddlGetHwInfo(Ddl::Destination::SIU, time);
  uint32_t siuStatus;
  try {
    siuStatus = ddlPrintStatus(Ddl::Destination::SIU, time);
  } catch (const Exception& e) {
    throw e;
  }

  return std::make_tuple(hwInfo, siuStatus);
}

std::vector<std::string> Crorc::ddlInterpretIfstw(uint32_t ifstw)
{
  /* for remote SIU/DIU status */
  std::string remoteStatus[] = { "Power On Reset", "Offline", "Online", "Waiting for PO",
                                 "Offline No Signal", "Offline LOS", "No Optical Signal", "undefined" };

  std::vector<std::string> statusStrings;
  int destination = ifstw & 0xf;
  uint32_t status = ifstw & Ddl::STMASK;

  auto appendString = [&](std::string string) mutable {
    statusStrings.push_back(string);
  };

  if (destination == Ddl::Destination::DIU) {

    if (status & Diu::DIU_LOOP)
      appendString("DIU is set in loop-back mode");
    if (status & Diu::ERROR_BIT) {
      appendString("DIU error bit(s) set");
      if (status & Diu::LOSS_SYNC)
        appendString("Loss of synchronization");
      if (status & Diu::TXOF)
        appendString("Transmit data/status overflow");
      if (status & Diu::RES1)
        appendString("Undefined DIU error");
      if (status & Diu::OSINFR)
        appendString("Ordered set in frame");
      if (status & Diu::INVRX)
        appendString("Invalid receive character in frame");
      if (status & Diu::CERR)
        appendString("CRC error");
      if (status & Diu::RES2)
        appendString("Undefined DIU error");
      if (status & Diu::DOUT)
        appendString("Data out of frame");
      if (status & Diu::IFDL)
        appendString("Illegal frame delimiter");
      if (status & Diu::LONG)
        appendString("Too long frame");
      if (status & Diu::RXOF)
        appendString("Received data/status overflow");
      if (status & Diu::FRERR)
        appendString("Error in receive frame");
    } else {
      appendString("DIU error bit(s) not set");
    }

    switch (status & Ddl::DIUSTMASK) {
      case Diu::PortState::TSTM:
        appendString("DIU port in PRBS Test Mode state");
        break;
      case Diu::PortState::POFF:
        appendString("DIU port in Power Off state");
        break;
      case Diu::PortState::LOS:
        appendString("DIU port in Offline Loss of Synchr. state");
        break;
      case Diu::PortState::NOSIG:
        appendString("DIU port in Offline No Signal state");
        break;
      case Diu::PortState::WAIT:
        appendString("DIU port in Waiting for Power Off state");
        break;
      case Diu::PortState::ONL:
        appendString("DIU port in Online state");
        break;
      case Diu::PortState::OFFL:
        appendString("DIU port in Offline state");
        break;
      case Diu::PortState::POR:
        appendString("DIU port in Power On Reset state");
        break;
    }

    int siuStatus = (status & Ddl::REMMASK) >> 15;
    appendString("Remote SIU/DIU port is in " + remoteStatus[siuStatus] + "state");

  } else { //destination == SIU

    if (status & Siu::ERROR_BIT) {
      appendString("SIU error bit(s) set:");
      if (status & Siu::LONGE)
        appendString("Too long event or read data block");
      if (status & Siu::IFEDS)
        appendString("Illegal FEE data/status");
      if (status & Siu::TXOF)
        appendString("Transmit FIFO overflow");
      if (status & Siu::IWDAT)
        appendString("Illegal write data word");
      if (status & Siu::OSINFR)
        appendString("Ordered set in frame");
      if (status & Siu::INVRX)
        appendString("Invalid character in receive frame");
      if (status & Siu::CERR)
        appendString("CRC error");
      if (status & Siu::DJLERR)
        appendString("DTCC or JTCC error");
      if (status & Siu::DOUT)
        appendString("Data out of receive frame");
      if (status & Siu::IFDL)
        appendString("Illegal frame delimiter");
      if (status & Siu::LONG)
        appendString("Too long receive frame");
      if (status & Siu::RXOF)
        appendString("Receive FIFO overflow");
      if (status & Siu::FRERR)
        appendString("Error in receive frame");
      if (status & Siu::LPERR)
        appendString("Link protocol error");
    } else {
      appendString("SIU error bit not set");
    }

    if (status & Siu::LBMOD)
      appendString("SIU in Loopback Mode");
    if (status & Siu::OPTRAN)
      appendString("One FEE transaction is open");

    switch (status & Ddl::SIUSTMASK) {
      case Siu::PortState::RESERV:
        appendString("SIU port in undefined state");
        break;
      case Siu::PortState::POFF:
        appendString("SIU port in Power Off state");
        break;
      case Siu::PortState::LOS:
        appendString("SIU port in Offline Loss of Synchr. state");
        break;
      case Siu::PortState::NOSIG:
        appendString("SIU port in Offline No Signal state");
        break;
      case Siu::PortState::WAIT:
        appendString("SIU port in Waiting for Power Off state");
        break;
      case Siu::PortState::ONL:
        appendString("SIU port in Online state");
        break;
      case Siu::PortState::OFFL:
        appendString("SIU port in Offline state");
        break;
      case Siu::PortState::POR:
        appendString("SIU port in Power On Reset state");
        break;
    }
  }

  return statusStrings;
}

} // namespace Crorc
} // namespace roc
} // namespace o2

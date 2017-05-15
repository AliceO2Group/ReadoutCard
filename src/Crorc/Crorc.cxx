/// \file Crorc.cxx
/// \brief Implementation of low level C-RORC functions
///
/// This scary looking file is a work in progress translation of the low-level C interface
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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
#include "Crorc/Constants.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"

using namespace std::chrono_literals;
using std::this_thread::sleep_for;
namespace b = boost;
namespace chrono = std::chrono;

/// Throws the given exception if the given status code is not equal to RORC_STATUS_OK
#define THROW_IF_BAD_STATUS(_status_code, _exception) \
  if (_status_code != Rorc::RORC_STATUS_OK) { \
    BOOST_THROW_EXCEPTION((_exception)); \
  }

/// Adds errinfo using the given status code and error message
#define ADD_ERRINFO(_status_code, _err_message) \
    << ErrorInfo::Message(_err_message) \
    << ErrorInfo::StatusCode(_status_code)

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
auto ST_DEST = [](auto fw) {return ((unsigned short)((fw) & 0xf));};
auto mask = [](auto a, auto b) {return a & b;};
auto incr15 = [](auto a) {return (((a) + 1) & 0xf);};
} // Anonymous namespace

namespace AliceO2
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

// TODO figure out what these are/do
constexpr int MAGIC_VALUE_0  = 0x80;
constexpr int MAGIC_VALUE_3  = 0x0100bddf;
constexpr int MAGIC_VALUE_13 = 0x03000000;
constexpr int MAGIC_VALUE_5  = 0x03000003;
constexpr int MAGIC_VALUE_12 = 0x0300001f;
constexpr int MAGIC_VALUE_8  = 0x03000020;
constexpr int MAGIC_VALUE_9  = 0x03000040;
constexpr int MAGIC_VALUE_2  = 0x03000050;
constexpr int MAGIC_VALUE_4  = 0x03000060;
constexpr int MAGIC_VALUE_6  = 0x03000070;
constexpr int MAGIC_VALUE_7  = 0x030000d0;
constexpr int MAGIC_VALUE_11 = 0x030000e8;
constexpr int MAGIC_VALUE_10 = 0x030000ff;
constexpr int MAGIC_VALUE_1  = 0x04000000;

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

void writeWord(RegisterReadWriteInterface& bar0, uint32_t address, int value)
{
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_9);
  writeStatusSleep(bar0, value);
  checkStatus(bar0);
}

/// Reads a 16-bit flash word and writes it into the given buffer
void readWord(RegisterReadWriteInterface& bar0, uint32_t address, char *data)
{
  writeStatusSleep(bar0, address);
  writeStatusSleep(bar0, MAGIC_VALUE_10);

  uint32_t stat = readStatus(bar0);
  data[0] = (stat & 0xFF00) >> 8;
  data[1] = stat & 0xFF;
}

void readRange(RegisterReadWriteInterface& bar0, int addressFlash, int wordNumber, std::ostream& out) {
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

void wait(RegisterReadWriteInterface& channel) {
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
  struct InterruptedException : public std::exception {};

  auto checkInterrupt = [&]{
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
  std::ifstream ifstream { dataFilePath };
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
          float perc = ((float) numberOfLinesRead / Flash::MAX_WORDS) * 100.0;
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

int Crorc::armDataGenerator(uint32_t initEventNumber, uint32_t initDataWord, GeneratorPattern::type dataPattern,
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

/* ddlSendCommand sends one command to the given link.
 * Parameters: dev      pointer to roc device. It defines the link
 *                      where the command will be sent
 *             dest     command destination: 0 RORC, 1 DIU, 2 SIU, 4 FEE.
 *                      if -1 then the full command is in the
 *                      command field
 *             command  command code
 *             transid  transaction id
 *             param    command parameter,
 *                      or the full command if dest == -1
 *             time     if > 0 then test if command can be sent and
 *                      wait as many cycles if necessary.
 *
 * Returns:
 *    RORC_STATUS_OK   (0)     if command sent
 *    RORC_TIMEOUT     (-64)   if the command can not be sent in timeout.
 *    RORC_LINK_NOT_ON (-4)    if destination > 1 and the link is not on
 */
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
  for (i = 0; i < time; i++){
    if (checkCommandRegister() == 0) {
      break;
    }
  }

  if (time && (i == time)) {
    BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Timed out sending DDL command"));
  }

  putCommandRegister(com);
}

void Crorc::ddlWaitStatus(long long int timeout)

/* Checks whether status mail box or register is not empty in timeout
 *
 * Parameters: prorc    pointer to pRORC device
 *             timeout  # of check cycles
 *
 * Returns:    # of executed cycles
 *
*/

{
  for (int i = 0; i < timeout; ++i) {
    if (checkRxStatus()) {
      return;
    }
  }
  BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Timed out waiting on DDL"));
}

StWord Crorc::ddlReadStatus()
{
  /* call ddlWaitStatus() before this routine
     if status timeout is needed */
  StWord stw;
  stw.stw = read(Rorc::C_DSR);
  //  printf("ddlReadStatus: status = %08lx\n", stw.stw);
  return stw;
}

StWord Crorc::ddlReadDiu(int transid, long long int time)
{
  /* prepare and send DDL command */
  int destination = Ddl::Destination::DIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time);
  ddlWaitStatus(time);
  StWord stw = ddlReadStatus();
  if (stw.part.code != Rorc::IFSTW || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
        Exception() << ErrorInfo::Message("Unexpected DIU STW (not IFSTW)")
            << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination))
            << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }

  stw = ddlReadCTSTW(transid, destination, time); // XXX Not sure why we do this...
  return stw;
}

StWord Crorc::ddlReadCTSTW(int transid, int destination, long long int time){
  ddlWaitStatus(time);
  StWord stw = ddlReadStatus();
  if ((stw.part.code != Rorc::CTSTW && stw.part.code != Rorc::ILCMD && stw.part.code != Rorc::CTSTW_TO)
      || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
        Exception() << ErrorInfo::Message("Unexpected STW (not CTSTW)")
        << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination))
        << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }
  return stw;
}

StWord Crorc::ddlReadSiu(int transid, long long int time)
{
  /* prepare and send DDL command */
  int destination = Ddl::Destination::SIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time);

  /* read and check the answer */
  ddlWaitStatus(time);
  StWord stw = ddlReadStatus();
  if (stw.part.code != Rorc::IFSTW || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
        Exception() << ErrorInfo::Message("Unexpected SIU STW (not IFSTW)")
            << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination))
            << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }

  stw = ddlReadStatus();
  if ((stw.part.code != Rorc::CTSTW && stw.part.code != Rorc::ILCMD && stw.part.code != Rorc::CTSTW_TO)
      || stw.part.trid != transid || stw.part.dest != destination) {
    BOOST_THROW_EXCEPTION(
        Exception() << ErrorInfo::Message("Unexpected SIU STW (not CTSTW)")
            << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination))
            << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }
  return stw;
}

/// Interpret DIU or SIU IFSTW
void ddlInterpretIFSTW(uint32_t ifstw)
{
  using namespace Ddl;
  using Table = std::vector<std::pair<uint32_t, const char*>>;

  const int destination = ST_DEST(ifstw);
  const uint32_t status = ifstw & STMASK;
  std::vector<std::string> messages;

  auto add = [&](const char* message){ messages.push_back(message); };

  auto checkTable = [&](uint32_t status, const Table& table) {
    for(const auto& pair : table) {
      if (mask(pair.first, status)) {
        add(pair.second);
      }
    }
  };

  auto checkTableExclusive = [&](uint32_t status, const Table& table) {
    for(const auto& pair : table) {
      if (mask(pair.first, status)) {
        add(pair.second);
        break;
      }
    }
  };

  if (destination == Destination::DIU) {
    using namespace Diu;
    const Table errorTable {
        {LOSS_SYNC, "Loss of synchronization"},
        {TXOF, "Transmit data/status overflow"},
        {RES1, "Undefined DIU error"},
        {OSINFR, "Ordered set in frame"},
        {INVRX, "Invalid receive character in frame"},
        {CERR, "CRC error"},
        {RES2, "Undefined DIU error"},
        {DOUT, "Data out of frame"},
        {IFDL, "Illegal frame delimiter"},
        {LONG, "Too long frame"},
        {RXOF, "Received data/status overflow"},
        {FRERR, "Error in receive frame"}};
    const Table portTable {
        {PortState::TSTM, "DIU port in PRBS Test Mode state"},
        {PortState::POFF, "DIU port in Power Off state"},
        {PortState::LOS, "DIU port in Offline Loss of Synchr. state"},
        {PortState::NOSIG, "DIU port in Offline No Signal state"},
        {PortState::WAIT, "DIU port in Waiting for Power Off state"},
        {PortState::ONL, "DIU port in Online state"},
        {PortState::OFFL, "DIU port in Offline state"},
        {PortState::POR, "DIU port in Power On Reset state"}};

    if (mask(status, DIU_LOOP)) {
      add("DIU is set in loop-back mode");
    }
    if (mask(status, ERROR_BIT)){
      checkTable(mask(status, DIUSTMASK), errorTable);
    }
    checkTableExclusive(status, portTable);
  } else {
    using namespace Siu;
    const Table okTable {
        {LBMOD, "SIU in Loopback Mode"},
        {OPTRAN, "One FEE transaction is open"}};
    const Table errorTable {
        {LONGE, "Too long event or read data block"},
        {IFEDS, "Illegal FEE data/status"},
        {TXOF, "Transmit FIFO overflow"},
        {IWDAT, "Illegal write data word"},
        {OSINFR, "Ordered set in frame"},
        {INVRX, "Invalid character in receive frame"},
        {CERR, "CRC error"},
        {DJLERR, "DTCC or JTCC error"},
        {DOUT, "Data out of receive frame"},
        {IFDL, "Illegal frame delimiter"},
        {LONG, "Too long receive frame"},
        {RXOF, "Receive FIFO overflow"},
        {FRERR, "Error in receive frame"},
        {LPERR, "Link protocol error"}};
    const Table portTable {
        {PortState::RESERV ,"SIU port in undefined state"},
        {PortState::POFF, "SIU port in Power Off state"},
        {PortState::LOS, "SIU port in Offline Loss of Synchr. state"},
        {PortState::NOSIG, "SIU port in Offline No Signal state"},
        {PortState::WAIT, "SIU port in Waiting for Power Off state"},
        {PortState::ONL, "SIU port in Online state"},
        {PortState::OFFL, "SIU port in Offline state"},
        {PortState::POR, "SIU port in Power On Reset state"}};

    if (mask(status, ERROR_BIT)){
      checkTable(status, errorTable);
    } else {
      checkTable(status, okTable);
    }
    checkTableExclusive(mask(status, SIUSTMASK), portTable);
  }
}

void Crorc::ddlResetSiu(int cycle, long long int time)
/* ddlResetSiu tries to reset the SIU.
 *             print  # if != 0 then print link status
 *             cycle  # of status checks
 *             time # of cycles to wait for command sending and replies
 * Returns:    SIU status word or -1 if no status word can be read
 */
{
  ddlSendCommand(Ddl::Destination::DIU, Ddl::SRST, 0,  0, time);
  ddlWaitStatus(time);
  ddlReadStatus();

  int transid = 0xf;

  for (int i = 0; i < cycle; ++i) {
    try {
      sleep_for(10ms);

      transid = incr15(transid);
      StWord stword = ddlReadDiu(transid, time);
      stword.stw &= Ddl::STMASK;

      if (stword.stw & Diu::ERROR_BIT){
        // TODO log error
        // ddlInterpretIFSTW(retlong, pref, suff, diuConfig.diuVersion);
        continue;
      }
      else if (mask(stword.stw, Siu::OPTRAN)){
        // TODO log error
        // ddlInterpretIFSTW(retlong, pref, suff, diuConfig.diuVersion);
        continue;
      }

      transid = incr15(transid);
      stword = ddlReadSiu(transid, time);
      stword.stw &= Ddl::STMASK;

      if (stword.stw & Diu::ERROR_BIT){
        // TODO log error
        // ddlInterpretIFSTW(retlong, pref, suff, diuConfig.diuVersion);
        continue;
      }

      return;
    } catch (const Exception& e) {
    }
  }

  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to reset SIU"));
}

void Crorc::resetCommand(int option, const DiuConfig& diuConfig)
{
  uint32_t command = 0;
  if (option & Rorc::Reset::DIU) {
    command |= Rorc::CcsrCommand::RESET_DIU;
  }
  if (option & Rorc::Reset::FF) {
    command |= Rorc::CcsrCommand::CLEAR_RXFF | Rorc::CcsrCommand::CLEAR_TXFF;
  }
  if (option & Rorc::Reset::FIFOS) {
    command |= Rorc::CcsrCommand::CLEAR_FIFOS;
  }
  if (option & Rorc::Reset::ERROR) {
    command |= Rorc::CcsrCommand::CLEAR_ERROR;
  }
  if (option & Rorc::Reset::COUNTERS) {
    command |= Rorc::CcsrCommand::CLEAR_COUNTERS;
  }
  if (command) {
    write(Rorc::C_CSR, (uint32_t) command);
  }
  if (option & Rorc::Reset::SIU) {
    putCommandRegister(Rorc::DcrCommand::RESET_SIU);
    ddlWaitStatus(Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec);
    ddlReadStatus();
  }
  if (!option || (option & Rorc::Reset::RORC)) {
    write(Rorc::RCSR, Rorc::RcsrCommand::RESET_CHAN);  //channel reset
  }
}

/* try to empty D-RORC's data FIFOs
               empty_time:  time-out value in usecs
 */
void Crorc::emptyDataFifos(int timeoutMicroseconds)
{
  auto endTime = chrono::steady_clock::now() + chrono::microseconds(timeoutMicroseconds);

  while (chrono::steady_clock::now() < endTime){
    if (!checkRxData()) {
      return;
    }
    write(Rorc::C_CSR, (uint32_t) Rorc::CcsrCommand::CLEAR_FIFOS);
  }

  if (checkRxData()) {
    BOOST_THROW_EXCEPTION(TimeoutException() << ErrorInfo::Message("Timed out emptying data FIFOs"));
  }
}


void Crorc::armDdl(int resetMask, const DiuConfig& diuConfig)
{
  auto reset = [&](uint32_t command){ resetCommand(command, diuConfig); };

  if (resetMask & Rorc::Reset::FEE){
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Command not allowed"));
  }
  if (resetMask & Rorc::Reset::SIU){
    ddlResetSiu(3, Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec);
  }
  if (resetMask & Rorc::Reset::LINK_UP){
    reset(Rorc::Reset::RORC);
    reset(Rorc::Reset::DIU);
    reset(Rorc::Reset::SIU);
    sleep_for(100ms);
    assertLinkUp();
    emptyDataFifos(100000);

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
  }
}

auto Crorc::initDiuVersion() -> DiuConfig
{
  int maxLoop = 1000;
  auto start = chrono::steady_clock::now();
  for (int i = 0; i < maxLoop; i++) {
    (void) checkRxStatus();
  };
  auto end = chrono::steady_clock::now();

  DiuConfig diuConfig;
  diuConfig.pciLoopPerUsec = double(maxLoop) / chrono::duration<double, std::micro>(end-start).count();
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

RxFreeFifoState Crorc::getRxFreeFifoState()
{
  uint32_t st = read(Rorc::C_CSR);
  if (st & Rorc::CcsrStatus::RXAFF_FULL) {
    return (RxFreeFifoState::FULL);
  } else if (st & Rorc::CcsrStatus::RXAFF_EMPTY) {
    return (RxFreeFifoState::EMPTY);
  } else {
    return (RxFreeFifoState::NOT_EMPTY);
  }
}

bool Crorc::isFreeFifoEmpty()
{
  return getRxFreeFifoState() == RxFreeFifoState::EMPTY;
}

void Crorc::assertFreeFifoEmpty()
{
  if (!isFreeFifoEmpty()) {
    BOOST_THROW_EXCEPTION(CrorcFreeFifoException() << ErrorInfo::Message("Free FIFO not empty")
        << ErrorInfo::PossibleCauses({"Previous DMA did not get/free all received pages"}));
  }
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

StWord Crorc::ddlSetSiuLoopBack(const DiuConfig& diuConfig){
  long long int timeout = diuConfig.pciLoopPerUsec * Ddl::RESPONSE_TIME;

  /* check SIU fw version */
  ddlSendCommand(Ddl::Destination::SIU, Ddl::IFLOOP, 0, 0, timeout);
  ddlWaitStatus(timeout);

  StWord stword = ddlReadStatus();
  if (stword.part.code == Rorc::ILCMD){
    /* illegal command => old version => send TSTMODE for loopback */
    ddlSendCommand(Ddl::Destination::SIU, Ddl::TSTMODE, 0, 0, timeout);
    ddlWaitStatus(timeout);
  }

  if (stword.part.code != Rorc::CTSTW) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Error setting SIU loopback"));
  }

  /* SIU loopback command accepted => check SIU loopback status */
  stword = ddlReadSiu(0, timeout);
  if (stword.stw & Siu::LBMOD) {
    return stword; // SIU loopback set
  }

  /* SIU loopback not set => set it */
  ddlSendCommand(Ddl::Destination::SIU, Ddl::IFLOOP, 0, 0, timeout);

  ddlWaitStatus(timeout);
  return ddlReadStatus();
}

void Crorc::setSiuLoopback(const DiuConfig& diuConfig)
{
  ddlSetSiuLoopBack(diuConfig);
}

void Crorc::startTrigger(const DiuConfig& diuConfig)
{
  uint64_t timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;
  ddlSendCommand(Ddl::Destination::FEE, Fee::RDYRX, 0, 0, timeout);
  ddlWaitStatus(timeout);
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

  // Try stopping twice
  try {
    rorcStopTrigger();
  } catch (const Exception& e) {
  }
  rorcStopTrigger();
}

void Crorc::setLoopbackOn(){
  if (!isLoopbackOn()) {
    toggleLoopback();
  }
}

void Crorc::setLoopbackOff()
{
  // switches off both loopback and stop_on_error
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

void Crorc::initReadoutContinuous(RegisterReadWriteInterface& bar2)
{
  // Based on:
  // https://gitlab.cern.ch/costaf/grorc_sw/blob/master/script/grorc_cont_readout.sh

  // this script works with the G-RORC firmware 3.16
  // SET the card in CONT MODE
  bar2.writeRegister(0x190/4, 0x02000000);
  // s_send_wide            <= s_reg(0);
  // s_gbt_tx_pol           <= s_reg(1);
  // s_gbt_tx_sel           <= s_reg(3 downto 2);
  // s_gbt_reset            <= s_reg(4);
  // s_rx_encoding          <= s_reg(8);
  // s_tx_encoding          <= s_reg(9);
  // s_read_gate            <= s_reg(12);
  // s_disable_bank         <= s_reg(17 downto 16);
  // s_bar_rd_ch(2).reg_194 <= s_reg;
  // Choose only BANK0
  bar2.writeRegister(0x194/4, 0x21005);
  // number of GBT words per event
  bar2.writeRegister(0x18c/4, 0x1f);
}

void Crorc::startReadoutContinuous(RegisterReadWriteInterface& bar2)
{
  // Based on:
  // https://gitlab.cern.ch/costaf/grorc_sw/blob/master/script/grorc_send_IDLE_SYNC.sh

  // SET IT AS CUSTOM PATTERN FOR 1 CC and open the GATE READOUT
  bar2.writeRegister(0x198/4, 0x60000001);
  bar2.writeRegister(0x198/4, 0x0);
}

void Crorc::initReadoutTriggered(RegisterReadWriteInterface&)
{
  BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
}

int getSerial(RegisterReadWriteInterface& bar0)
{
  // Reading the FLASH.
  uint32_t address = Rorc::Serial::FLASH_ADDRESS;
  Flash::init(bar0, address);

  // Setting the address to the serial number's (SN) position. (Actually it's the position
  // one before the SN's, because we need even position and the SN is at odd position.
  std::array<char, Rorc::Serial::LENGTH + 1> data { '\0' };
  address += (Rorc::Serial::POSITION - 1) / 2;
  for (int i = 0; i < Rorc::Serial::LENGTH; i += 2, address++) {
    Flash::readWord(bar0, address, &data[i]);
    if ((data[i] == '\0') || (data[i + 1] == '\0')) {
      break;
    }
  }

  // We don't use the first character for the conversion, since we started reading one byte before the serial number
  // location in the flash
  return boost::lexical_cast<int>(data.data() + 1, Rorc::Serial::LENGTH);
}

} // namespace Crorc
} // namespace roc
} // namespace AliceO2

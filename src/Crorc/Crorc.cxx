/// \file Crorc.cxx
/// \brief Implementation of low level C-RORC functions
///
/// This scary looking file is a work in progress translation of the low-level C interface
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Crorc.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <boost/format.hpp>
#include "Crorc/Constants.h"
#include "c/rorc/aux.h"
#include "c/rorc/rorc_macros.h"
#include "ExceptionInternal.h"
#include "RORC/RegisterReadWriteInterface.h"

namespace b = boost;

/// Throws the given exception if the given status code is not equal to RORC_STATUS_OK
#define THROW_IF_BAD_STATUS(_status_code, _exception) \
  if (_status_code != Rorc::RORC_STATUS_OK) { \
    BOOST_THROW_EXCEPTION((_exception)); \
  }

/// Adds errinfo using the given status code and error message
#define ADD_ERRINFO(_status_code, _err_message) \
    << ErrorInfo::Message(_err_message) \
    << ErrorInfo::StatusCode(_status_code)

namespace AliceO2
{
namespace Rorc
{
namespace Crorc
{
namespace Flash
{
namespace
{
constexpr int MAX_WAIT = 1000000;
constexpr int DATA_STATUS = Rorc::F_IFDSR;
constexpr int ADDRESS = Rorc::F_IADR;
constexpr int READY = Rorc::F_LRD;

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

unsigned readStatus(RegisterReadWriteInterface& channel, int sleept = 1)
{
  unsigned stat;

  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_1);
  usleep(sleept);
  stat = channel.readRegister(ADDRESS);

  return (stat);
}

uint32_t init(RegisterReadWriteInterface& channel, int sleept = 10)
{
  // Clear Status register
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_2);
  usleep(10*sleept);

  // Set ASYNCH mode (Configuration Register 0xBDDF)
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_3);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_4);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_5);
  usleep(sleept);

  // Read Status register
  uint32_t address = 0x01000000;
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_6);
  usleep(sleept);

  uint32_t stat = readStatus(channel);
  return stat;
}

void checkStatus(RegisterReadWriteInterface& channel, int timeout = MAX_WAIT)
{
  unsigned stat;
  int i = 0;

  while (1)
  {
    stat = readStatus(channel);
    if (stat == MAGIC_VALUE_0) {
      break;
    }
    if (timeout && (++i >= timeout)) {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Bad flash status"));
    }
    usleep(100);
  }
}

void unlockBlock(RegisterReadWriteInterface& channel, uint32_t address, int sleept = 10)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_4);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_7);
  usleep(sleept);

  checkStatus(channel);
}

void eraseBlock(RegisterReadWriteInterface& channel, uint32_t address, int sleept = 10)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_8);
  usleep(sleept);

  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_7);
  usleep(sleept);

  checkStatus(channel);
}

void writeWord(RegisterReadWriteInterface& channel, uint32_t address, int value, int sleept)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_9);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, value);
  usleep(sleept);
  checkStatus(channel);
}

void readWord(RegisterReadWriteInterface& channel, uint32_t address, uint8_t *data, int sleept)
{
  channel.writeRegister(DATA_STATUS, address);
  usleep(sleept);
  channel.writeRegister(DATA_STATUS, MAGIC_VALUE_10);
  usleep(sleept);
  uint32_t stat = readStatus(channel, sleept);
  *data = (stat & 0xFF00) >> 8;
  *(data+1) = stat & 0xFF;
}

void readRange(RegisterReadWriteInterface& channel, int addressFlash, int wordNumber, std::ostream& out) {
  for (int i = addressFlash; i < (addressFlash + wordNumber); ++i) {
    uint32_t address = i;
    address = 0x01000000 | address;

    channel.writeRegister(DATA_STATUS, address);
    usleep(50);

    channel.writeRegister(DATA_STATUS, MAGIC_VALUE_10);
    usleep(50);

    channel.writeRegister(DATA_STATUS, MAGIC_VALUE_1);
    usleep(50);

    uint32_t status = channel.readRegister(ADDRESS);
    uint32_t status2 = channel.readRegister(READY);

    out << b::format("%5u  %d\n") % status % status2;
  }
}

void wait(RegisterReadWriteInterface& channel) {
  int status = 0;
  while (status == 0) {
    status = channel.readRegister(READY);
  }
  usleep(1);
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
    if (interrupt != nullptr) {
      if (interrupt->load(std::memory_order_relaxed)) {
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
    out << "Initializing flash\n";
    uint64_t status = Flash::init(channel);
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

      // set address WRITE IN THE FLASH
      writeWait(Flash::DATA_STATUS, address);

      // SET Buffer Program
      writeWait(Flash::DATA_STATUS, Flash::MAGIC_VALUE_11);

      // READ STATUS REGISTER
      Flash::checkStatus(channel);

      // Write 32 words (31+1) (31 0x1f)
      writeWait(Flash::DATA_STATUS, Flash::MAGIC_VALUE_12);

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

        int hexValue = Flash::MAGIC_VALUE_13;
        int hex = std::atoi(data);
        hexValue += hex;

        writeWait(Flash::DATA_STATUS, address);
        writeWait(Flash::DATA_STATUS, hexValue);

        address++;
        numberOfLinesRead++;
        if (numberOfLinesRead % 1000 == 0) {
          constexpr float MAX_WORDS { Flash::MAX_WORDS };
          float perc = ((float) numberOfLinesRead / MAX_WORDS) * 100.0;
          out << format("\r  Progress  %1.1f%%") % perc << std::flush;
        }
      }

      writeWait(Flash::DATA_STATUS, Flash::MAGIC_VALUE_7);
      writeWait(Flash::DATA_STATUS, Flash::MAGIC_VALUE_1);

      status = channel.readRegister(Flash::ADDRESS);
      while (status != 0x80) {
        checkInterrupt();

        writeWait(Flash::DATA_STATUS, Flash::MAGIC_VALUE_1);
        status = readWait(Flash::ADDRESS);

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
    channel.writeRegister(Flash::DATA_STATUS, Flash::MAGIC_VALUE_6);
    usleep(1);
    Flash::checkStatus(channel);
  } catch (const InterruptedException& e) {
    out << "Flash programming interrupted\n";
  }
}

int Crorc::armDataGenerator(uint32_t initEventNumber, uint32_t initDataWord, GeneratorPattern::type dataPattern,
    int dataSize, int seed)
{
  int roundedLen = -1;
  int eventLen = dataSize / 4;

  {
//    uint32_t           initEventNumber
//    uint32_t           initDataWord,
//    int             dataPattern,
//    int             eventLen,
//    int             seed,
//    int             *rounded_len

    unsigned long blockLen = 0;

    if ((eventLen < 1) || (eventLen >= 0x00080000)) {
      BOOST_THROW_EXCEPTION(CrorcArmDataGeneratorException()
          << ErrorInfo::Message("Failed to arm data generator; invalid event length")
          << ErrorInfo::GeneratorEventLength(eventLen));
    }

    roundedLen = eventLen;

    if (seed) {
      /* round to the nearest lower power of two */
      roundedLen = roundPowerOf2(eventLen);
      blockLen = ((roundedLen - 1) << 4) | dataPattern;
      blockLen |= 0x80000000;
      write(Rorc::C_DG2, seed);
    }
    else{
      blockLen = ((eventLen - 1) << 4) | dataPattern;
      write(Rorc::C_DG2, initDataWord);
    }

    write(Rorc::C_DG1, blockLen);
    write(Rorc::C_DG3, initEventNumber);
  }

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
  write(Rorc::C_CSR, Rorc::DRORC_CMD_START_DG);
}

void Crorc::stopDataGenerator()
{
  write(Rorc::C_CSR, Rorc::DRORC_CMD_STOP_DG);
}

void Crorc::stopDataReceiver()
{
  if (read(Rorc::C_CSR) & Rorc::DRORC_CMD_DATA_RX_ON_OFF) {
    write(Rorc::C_CSR, Rorc::DRORC_CMD_DATA_RX_ON_OFF);
  }
}


void Crorc::ddlSendCommand(
                   int             dest,
                   uint32_t           command,
                   int             transid,
                   uint32_t           param,
                   long long int   time)

/* ddlSendCommand sends one command to the given link.
 * Parameters: dev      pointer to Rorc device. It defines the link
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

{
  uint32_t com;
  int destination;

  if (dest == -1) {
    com = command;
    destination = com & 0xf;
  }
  else {
    destination = dest & 0xf;
    com = destination + ((command & 0xf)<<4) +
      ((transid & 0xf)<<8) + ((param & 0x7ffff)<<12);
  }

  if (destination > Ddl::Destination::DIU){
    assertLinkUp();
  }

  long long int i;
  for (i = 0; i < time; i++){
    if (checkCommandRegister() == 0) {
      break;
    }
  }

  if (time && (i == time)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Timed out sending DDL command"));
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
  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Timed out waiting on DDL"));
}

stword_t Crorc::ddlReadStatus()
{
  stword_t stw;
  /* call ddlWaitStatus() before this routine
     if status timeout is needed */
  stw.stw = read(Rorc::C_DSR);
  //  printf("ddlReadStatus: status = %08lx\n", stw.stw);

  return stw;
}

stword_t Crorc::ddlReadDiu(int transid, long long int time)
{
  /* prepare and send DDL command */
  int destination = Ddl::Destination::DIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time);
  ddlWaitStatus(time);
  stword_t stw = ddlReadStatus();
  if (stw.part.code != Rorc::IFSTW ||
      stw.part.trid != transid   ||
      stw.part.dest != destination){
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Unexpected DIU STW (not IFSTW)")
        << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination))
        << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }

  stw = ddlReadCTSTW(transid, destination, time); // XXX Not sure why we do this...
  return stw;
}

stword_t Crorc::ddlReadCTSTW(int transid, int destination, long long int time){
  ddlWaitStatus(time);
  stword_t stw = ddlReadStatus();
  if ((stw.part.code != Rorc::CTSTW &&
       stw.part.code != Rorc::ILCMD &&
       stw.part.code != Rorc::CTSTW_TO) ||
       stw.part.trid != transid   ||
      stw.part.dest != destination){
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Unexpected STW (not CTSTW)")
        << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination))
        << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }
  return stw;
}

stword_t Crorc::ddlReadSiu(int transid, long long int time)
{
  /* prepare and send DDL command */
  int destination = Ddl::Destination::SIU;
  ddlSendCommand(destination, Rorc::RandCIFST, transid, 0, time);

  /* read and check the answer */
  ddlWaitStatus(time);
  stword_t stw = ddlReadStatus();
  if (stw.part.code != Rorc::IFSTW ||
      stw.part.trid != transid   ||
      stw.part.dest != destination){
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Unexpected SIU STW (not IFSTW)")
        << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::IFSTW % destination))
        << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }

  stw = ddlReadStatus();
  if ((stw.part.code != Rorc::CTSTW &&
       stw.part.code != Rorc::ILCMD &&
       stw.part.code != Rorc::CTSTW_TO) ||
       stw.part.trid != transid   ||
       stw.part.dest != destination){
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Unexpected SIU STW (not CTSTW)")
        << ErrorInfo::StwExpected(b::str(b::format("0x00000%x%x%x") % transid % Rorc::CTSTW % destination))
        << ErrorInfo::StwReceived(b::str(b::format("0x%08lx") % stw.stw)));
  }
  return stw;
}

/// Interpret DIU or SIU IFSTW
void ddlInterpretIFSTW(uint32_t ifstw, const char *pref, const char *suff)
{
  int destination;
  unsigned long status;
  destination = ST_DEST(ifstw);

  using namespace Ddl;
  using namespace Diu;
  using namespace Siu;

  status = ifstw & Ddl::STMASK;
  if (destination == Ddl::Destination::DIU) {
    if (mask(status, DIU_LOOP))
      printf("%sDIU is set in loop-back mode%s", pref, suff);
    if (mask(status, ERROR_BIT)){
      if (strlen(pref) == 0)
        printf("%sDIU error bit(s) set:%s", pref, suff);
      if (mask(status, LOSS_SYNC))
        printf("%s Loss of synchronization%s", pref, suff);
      if (mask(status, D_TXOF))
        printf("%s Transmit data/status overflow%s", pref, suff);
      if (mask(status, D_RES1))
        printf("%s Undefined DIU error%s", pref, suff);
      if (mask(status, D_OSINFR))
        printf("%s Ordered set in frame%s", pref, suff);
      if (mask(status, D_INVRX))
        printf("%s Invalid receive character in frame%s", pref, suff);
      if (mask(status, D_CERR))
        printf("%s CRC error%s", pref, suff);
      if (mask(status, D_RES2))
        printf("%s Undefined DIU error%s", pref, suff);
      if (mask(status, D_DOUT))
        printf("%s Data out of frame%s", pref, suff);
      if (mask(status, D_IFDL))
        printf("%s Illegal frame delimiter%s", pref, suff);
      if (mask(status, D_LONG))
        printf("%s Too long frame%s", pref, suff);
      if (mask(status, D_RXOF))
        printf("%s Received data/status overflow%s", pref, suff);
      if (mask(status, D_FRERR))
        printf("%s Error in receive frame%s", pref, suff);
    }

    switch (mask(status, Ddl::DIUSTMASK)){
      case DIU_TSTM:
        printf("%sDIU port in PRBS Test Mode state%s", pref, suff); break;
      case DIU_POFF:
        printf("%sDIU port in Power Off state%s", pref, suff); break;
      case DIU_LOS:
        printf("%sDIU port in Offline Loss of Synchr. state%s", pref, suff); break;
      case DIU_NOSIG:
        printf("%sDIU port in Offline No Signal state%s", pref, suff); break;
      case DIU_WAIT:
        printf("%sDIU port in Waiting for Power Off state%s", pref, suff); break;
      case DIU_ONL:
        printf("%sDIU port in Online state%s", pref, suff); break;
      case DIU_OFFL:
        printf("%sDIU port in Offline state%s", pref, suff); break;
      case DIU_POR:
        printf("%sDIU port in Power On Reset state%s", pref, suff); break;
    }

    int siuStatus = (status & Ddl::REMMASK) >> 15;
    // XXX investigate remoteStatus variable
    //printf("%sremote SIU/DIU port in %s state%s", pref, remoteStatus[siuStatus], suff);
  }
  else  /* SIU */
  {
    if (mask(status, ERROR_BIT)){
      if (strlen(pref) == 0)
        printf("%sSIU error bit(s) set:%s", pref, suff);
      if (mask(status, S_LONGE))
        printf("%s Too long event or read data block%s", pref, suff);
      if (mask(status, S_IFEDS))
        printf("%s Illegal FEE data/status%s", pref, suff);
      if (mask(status, S_TXOF))
        printf("%s Transmit FIFO overflow%s", pref, suff);
      if (mask(status, S_IWDAT))
        printf("%s Illegal write data word%s", pref, suff);
      if (mask(status, S_OSINFR))
        printf("%s Ordered set in frame%s", pref, suff);
      if (mask(status, S_INVRX))
        printf("%s Invalid character in receive frame%s", pref, suff);
      if (mask(status, S_CERR))
        printf("%s CRC error%s", pref, suff);
      if (mask(status, S_DJLERR))
        printf("%s DTCC or JTCC error%s", pref, suff);
      if (mask(status, S_DOUT))
        printf("%s Data out of receive frame%s", pref, suff);
      if (mask(status, S_IFDL))
        printf("%s Illegal frame delimiter%s", pref, suff);
      if (mask(status, S_LONG))
        printf("%s Too long receive frame%s", pref, suff);
      if (mask(status, S_RXOF))
        printf("%s Receive FIFO overflow%s", pref, suff);
      if (mask(status, S_FRERR))
        printf("%s Error in receive frame%s", pref, suff);
      if (mask(status, S_LPERR))
        printf("%s Link protocol error%s", pref, suff);
    }
    else
      printf("%sSIU error bit not set%s", pref, suff);

    if (mask(status, S_LBMOD))
      printf("%sSIU in Loopback Mode%s", pref, suff);
    if (mask(status, S_OPTRAN))
      printf("%sOne FEE transaction is open%s", pref, suff);

    if      (mask(status, SIUSTMASK) == SIU_RESERV) {
      printf("%sSIU port in undefined state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_POFF) {
      printf("%sSIU port in Power Off state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_LOS) {
      printf("%sSIU port in Offline Loss of Synchr. state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_NOSIG) {
      printf("%sSIU port in Offline No Signal state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_WAIT ) {
      printf("%sSIU port in Waiting for Power Off state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_ONL ) {
      printf("%sSIU port in Online state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_OFFL) {
      printf("%sSIU port in Offline state%s", pref, suff);
    } else if (mask(status, SIUSTMASK) == SIU_POR) {
      printf("%sSIU port in Power On Reset state%s", pref, suff);
    }
  }
}

void Crorc::ddlResetSiu(int print, int cycle, long long int time, const DiuConfig& diuConfig)
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

  int trial = cycle + 1;
  int transid = 0xf;

  while (trial > 0){
    trial--;
    try {
      usleep(10000);   // sleep 10 msec

      transid = incr15(transid);
      stword_t stword = ddlReadDiu(transid, time);
      stword.stw &= Ddl::STMASK;

      if (stword.stw & Diu::ERROR_BIT){
        // TODO log error
        // ddlInterpretIFSTW(retlong, pref, suff, diuConfig.diuVersion);
        continue;
      }
      else if (mask(stword.stw, Siu::S_OPTRAN)){
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
      continue;
    }
  }

  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to reset SIU"));
}

void Crorc::resetCommand(int option, const DiuConfig& diuConfig){
  int prorc_cmd;
  uint64_t timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;
  prorc_cmd = 0;
  if (option & Rorc::Reset::DIU)
    prorc_cmd |= Rorc::DRORC_CMD_RESET_DIU;
  if (option & Rorc::Reset::FF)
    prorc_cmd |= Rorc::DRORC_CMD_CLEAR_RXFF | Rorc::DRORC_CMD_CLEAR_TXFF;
  if (option & Rorc::Reset::FIFOS)
    prorc_cmd |= Rorc::DRORC_CMD_CLEAR_FIFOS;
  if (option & Rorc::Reset::ERROR)
    prorc_cmd |= Rorc::DRORC_CMD_CLEAR_ERROR;
  if (option & Rorc::Reset::COUNTERS)
    prorc_cmd |= Rorc::DRORC_CMD_CLEAR_COUNTERS;
  if (prorc_cmd)   // any reset
    {
      write(Rorc::C_CSR, (uint32_t) prorc_cmd);
    }
    if (option & Rorc::Reset::SIU)
    {
      putCommandRegister(Rorc::PRORC_CMD_RESET_SIU);
      ddlWaitStatus(timeout);
      ddlReadStatus();
    }
    if (!option || (option & Rorc::Reset::RORC))
    {
      write(Rorc::RCSR, Rorc::DRORC_CMD_RESET_CHAN);  //channel reset
    }
  }

/* try to empty D-RORC's data FIFOs
               empty_time:  time-out value in usecs
 */
void Crorc::emptyDataFifos(int empty_time)
{
  struct timeval start_tv, now_tv;
  int dsec, dusec;
  double dtime;
  gettimeofday(&start_tv, NULL);
  dtime = 0;
  while (dtime < empty_time){
    if (!checkRxData()) {
      return;
    }

    write(Rorc::C_CSR, (uint32_t) Rorc::DRORC_CMD_CLEAR_FIFOS);
    gettimeofday(&now_tv, NULL);
    elapsed(&now_tv, &start_tv, &dsec, &dusec);
    dtime = (double)dsec * 1000000 + (double)dusec;
  }

  if (checkRxData()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Timed out emptying data FIFOs"));
  }
}


void Crorc::armDdl(int resetMask, const DiuConfig& diuConfig)
{
  int print = 0; // -1;
  int stop = 1;
  long long int TimeOut;

  auto reset = [&](uint32_t command){ resetCommand(command, diuConfig); };

  auto pciLoopPerUsec = diuConfig.pciLoopPerUsec;

  TimeOut = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;

  if (resetMask & Rorc::Reset::FEE){
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Command not allowed"));
  }
  if (resetMask & Rorc::Reset::SIU){
    ddlResetSiu(0, 3, TimeOut, diuConfig);
  }
  if (resetMask & Rorc::Reset::LINK_UP){
    reset(Rorc::Reset::RORC);
    reset(Rorc::Reset::DIU);
    reset(Rorc::Reset::SIU);
    usleep(100000);
    assertLinkUp();
    emptyDataFifos(100000);

    reset(Rorc::Reset::SIU);
    reset(Rorc::Reset::DIU);
    reset(Rorc::Reset::RORC);
    usleep(100000);
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
  DiuConfig diuConfig;

  struct timeval tv1, tv2;
  int dsec, dusec;
  double dtime, max_loop;
  int i;

  max_loop = 1000000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++){};

  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * 1000000 + (double)dusec;
  diuConfig.loopPerUsec = (double)max_loop/dtime;
  if (diuConfig.loopPerUsec < 1) {
    diuConfig.loopPerUsec = 1;
  }
  // printf("memory loop_per_usec: %lld\n", prorc_dev->loop_per_usec);

  /* calibrate PCI loop time for time-outs */
  max_loop = 1000;
  gettimeofday(&tv1, NULL);

  for (i = 0; i < max_loop; i++) {
    (void) checkRxStatus(); // XXX Cast to void to explicitly discard returned value
  }

  gettimeofday(&tv2, NULL);
  elapsed(&tv2, &tv1, &dsec, &dusec);
  dtime = (double)dsec * 1000000 + (double)dusec;
  diuConfig.pciLoopPerUsec = (double)max_loop/dtime;
  return diuConfig;
}

bool Crorc::isLinkUp()
{
  return !((read(Rorc::C_CSR) & Rorc::DRORC_STAT_LINK_DOWN));
}

void Crorc::assertLinkUp()
{
  if (!isLinkUp()) {
    BOOST_THROW_EXCEPTION(CrorcCheckLinkException() << ErrorInfo::Message("Link was not up"));
  }
}

void Crorc::siuCommand(int command, const DiuConfig& diuConfig)
{
  ddlReadSiu(command, Ddl::RESPONSE_TIME);
}

void Crorc::diuCommand(int command, const DiuConfig& diuConfig)
{
  ddlReadDiu(command, Ddl::RESPONSE_TIME);
}

int Crorc::getRxFreeFifoState()
{
  uint32_t st = read(Rorc::C_CSR);
  if (st & Rorc::DRORC_STAT_RXAFF_FULL)
    return (Rorc::RORC_FF_FULL);
  else if (st & Rorc::DRORC_STAT_RXAFF_EMPTY)
    return (Rorc::RORC_FF_EMPTY);
  else
    return (Rorc::RORC_STATUS_OK);  //Not Empty
}

bool Crorc::isFreeFifoEmpty()
{
  return getRxFreeFifoState() == Rorc::RORC_FF_EMPTY;
}

void Crorc::assertFreeFifoEmpty()
{
  int returnCode = getRxFreeFifoState();
  if (returnCode != Rorc::RORC_FF_EMPTY) {
    BOOST_THROW_EXCEPTION(
        CrorcFreeFifoException() ADD_ERRINFO(returnCode, "Free FIFO not empty")
        << ErrorInfo::PossibleCauses({"Previous DMA did not get/free all received pages"}));
  }
}

void Crorc::startDataReceiver(uintptr_t readyFifoBusAddress, int rorcRevision)
{
  int fw_major, fw_minor;
  unsigned long fw;
  write(Rorc::C_RRBAR, (readyFifoBusAddress & 0xffffffff));
  if (rorcRevision >= Rorc::RORC_REVISION_DRORC2){
    fw = read(Rorc::RFID);
    fw_major = rorcFWVersMajor(fw);
    fw_minor = rorcFWVersMinor(fw);
    if ((rorcRevision >= Rorc::RORC_REVISION_PCIEXP) ||
        (fw_major > 2) || ((fw_major == 2) && (fw_minor >= 16))){
#if __WORDSIZE > 32
      write(Rorc::C_RRBX, (readyFifoBusAddress >> 32));
#else
      write(Rorc::C_RRBX, 0x0);
#endif
    }
  }
  if (!(read(Rorc::C_CSR) & Rorc::DRORC_CMD_DATA_RX_ON_OFF)) {
    write(Rorc::C_CSR, Rorc::DRORC_CMD_DATA_RX_ON_OFF);
  }
}

stword_t Crorc::ddlSetSiuLoopBack(const DiuConfig& diuConfig){
  long long int timeout = diuConfig.pciLoopPerUsec * Ddl::RESPONSE_TIME;

  /* check SIU fw version */
  ddlSendCommand(Ddl::Destination::SIU, Ddl::IFLOOP, 0, 0, timeout);
  ddlWaitStatus(timeout);

  stword_t stword = ddlReadStatus();
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
  if (stword.stw & Siu::S_LBMOD) {
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
  stword_t stw = ddlReadStatus();
}

void Crorc::stopTrigger(const DiuConfig& diuConfig)
{
  uint64_t timeout = Ddl::RESPONSE_TIME * diuConfig.pciLoopPerUsec;

  auto rorcStopTrigger = [&] {
    ddlSendCommand(Ddl::Destination::FEE, Fee::EOBTR, 0, 0, timeout);
    ddlWaitStatus(timeout);
    stword_t stw = ddlReadStatus();
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
  return (read(Rorc::C_CSR) & Rorc::DRORC_CMD_LOOPB_ON_OFF) == 0 ? false : true;
}

/// Not sure
void Crorc::toggleLoopback()
{
  write(Rorc::C_CSR, Rorc::DRORC_CMD_LOOPB_ON_OFF);
}

uint32_t Crorc::checkCommandRegister()
{
  return read(Rorc::C_CSR) & Rorc::DRORC_STAT_CMD_NOT_EMPTY;
}

void Crorc::putCommandRegister(uint32_t command)
{
  write(Rorc::C_DCR, command);
}

uint32_t Crorc::checkRxStatus()
{
  return read(Rorc::C_CSR) & Rorc::DRORC_STAT_RXSTAT_NOT_EMPTY;
}

uint32_t Crorc::checkRxData()
{
  return read(Rorc::C_CSR) & Rorc::DRORC_STAT_RXDAT_NOT_EMPTY;
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

void Crorc::initReadoutTriggered(RegisterReadWriteInterface& bar2)
{
  BOOST_THROW_EXCEPTION(std::runtime_error("not implemented"));
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

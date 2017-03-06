/// \file ChannelUtilityImpl.cxx
/// \brief Implementation of the ChannelUtilityImpl helper functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelUtilityImpl.h"
#include <iostream>
#include <functional>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "ChannelPaths.h"
#include "Cru/CruRegisterIndex.h"

namespace AliceO2 {
namespace Rorc {
namespace ChannelUtility {

using std::endl;
namespace b = boost;

namespace {

/// Helper function for the "print FIFO" functions
void printTable(std::ostream& os, const std::string& title, const std::string& header, int size,
    std::function<void(int i)> printRow)
{
  int headerInterval = 32;
  auto lineFat = std::string(header.length(), '=') + '\n';
  auto lineThin = std::string(header.length(), '-') + '\n';

  os << "  " << title << "\n";
  os << lineFat << header << lineThin;

  for (int i = 0; i < size; ++i) {
    if ((i != 0) && ((i % headerInterval) == 0)) {
      // Add another header every x rows to make long tables more readable
      os << lineThin << header << lineThin;
    }
    printRow(i);
  }

  os << lineFat;
}

/// Helper function for the "sanity check" functions
void registerCheck(std::ostream& os, int writeValue, int writeAddress, int readValue, int readAddress)
{
  auto formatFailure = " [FAILURE] wrote 0x%x to 0x%x, read 0x%x from 0x%x";
  auto formatSuccess = " [success] wrote 0x%x to 0x%x, read 0x%x from 0x%x";
  auto format = (readValue == writeValue) ? formatSuccess : formatFailure;
  os << b::str(b::format(format) % writeValue % writeAddress % readValue % readAddress) << endl;
}

/// Helper function for the "sanity check" functions
void registerReadWriteCheck(RegisterReadWriteInterface* channel, std::ostream& os, int writeValue,
    int writeAddress, int readAddress)
{
  int writeIndex = writeAddress / 4;
  int readIndex = readAddress / 4;

  channel->writeRegister(writeIndex, writeValue);
  int readValue = channel->readRegister(readIndex);
  registerCheck(os, writeValue, writeAddress, readValue, readAddress);
}

void rorcCleanupState(const ChannelPaths& paths)
{
  using boost::filesystem::remove;
  remove(paths.state().c_str());
  remove(paths.fifo().c_str());
  remove(paths.lock().c_str());
  boost::interprocess::named_mutex::remove(paths.namedMutex().c_str());
}

} // Anonymous namespace

/// Prints the C-RORC Ready FIFO
void printCrorcFifo(ReadyFifo* fifo, std::ostream& os)
{
  auto header = b::str(b::format(" %-4s %-14s %-14s %-14s %-14s\n")
      % "#" % "Length (hex)" % "Status (hex)" % "Length (dec)" % "Status (dec)");

  printTable(os, "C-RORC READY FIFO", header, fifo->entries.size(), [&](int i) {
    // Note that, because the values are volatile, we need to make explicit non-volatile copies
    auto length = fifo->entries[i].length;
    auto status = fifo->entries[i].status;
    os << b::str(b::format(" %-4d %14x %14x %14d %14d\n") % i % length % status % length % status);
  });
}

/// Prints the CRU status & descriptor table
void printCruFifo(CruFifoTable* fifo, std::ostream& os)
{
  {
    auto header = b::str(b::format(" %-4s %-14s %-14s\n")
        % "#" % "Status (hex)" % "Status (dec)");

    printTable(os, "CRU STATUS TABLE", header, fifo->statusEntries.size(), [&](int i) {
      // Note that, because the values are volatile, we need to make explicit non-volatile copies
      auto status = fifo->statusEntries[i].status;
      os << b::str(b::format(" %-4d %14x %14d\n") % i % status % status);
    });
  }

  {
    auto header = b::str(b::format(" %-4s %-14s %-14s %-14s %-14s %-14s\n")
        % "#" % "Ctrl (hex)" % "SrcLo (hex)" % "SrcHi (hex)" % "DstLo (hex)" % "DstHi (hex)");

    printTable(os, "CRU DESCRIPTOR TABLE", header, fifo->descriptorEntries.size(), [&](int i) {
      auto d = fifo->descriptorEntries[i];
      // Note that, because the values are volatile, we need to make explicit non-volatile copies
      auto ctrl = d.ctrl;
      auto sl = d.srcLow;
      auto sh = d.srcHigh;
      auto dl = d.dstLow;
      auto dh = d.dstHigh;
      os << b::str(b::format(" %-4d %14x %14x %14x %14x %14x\n") % i % ctrl % sl % sh % dl % dh);
    });
  }
}

void crorcSanityCheck(std::ostream& os, RegisterReadWriteInterface* channel)
{
  {
    // A register that can be freely written to and read from

    int value = 0x1234abcd;
    int address = 0x1f4;

    os << "# Read/write register\n";
    registerReadWriteCheck(channel, os, value, address, address);
  }

  {
    // A pair of registers. A value can be written to one, and the same value should come out of the other

    int addressWrite = 0x1f8;
    int addressRead = 0x1fc;

    os << "# Readback register pair\n";
    registerReadWriteCheck(channel, os, 0x0, addressWrite, addressRead);
    registerReadWriteCheck(channel, os, 0x1, addressWrite, addressRead);
  }
}

void cruSanityCheck(std::ostream& os, RegisterReadWriteInterface* channel)
{
  {
    // Writing to the LED register has been known to crash and reboot the machine if the CRU is in a bad state.
    // That makes it a good part of this sanity test..?

    int ledAddress = CruRegisterIndex::LED_STATUS;
    int valueOn = 0xff;
    int valueOff = 0x00;

    os << "# Turning LEDs on\n";
    registerReadWriteCheck(channel, os, valueOn, ledAddress, ledAddress);
    os << "# Turning LEDs off\n";
    registerReadWriteCheck(channel, os, valueOff, ledAddress, ledAddress);
  }

  /* FIFO no longer exists in latest firmware
  {
    // The CRU has a little debug register FIFO thing that we can use to check if simple register writing and
    // reading is working properly.
    // We should be able to push values by writing to 'addressPush' and pop by reading from 'addressPop'

    int indexPush = CruRegisterIndex::DEBUG_FIFO_PUSH;
    int indexPop = CruRegisterIndex::DEBUG_FIFO_POP;
    int addressPush = indexPush * 4;
    int addressPop = indexPop * 4;

    int valuesToPush = 4;
    auto getValue = [](int i) { return 1 << i; };

    os << "# Debug FIFO push/pop register pair\n";
    for (int i = 0; i < valuesToPush; ++i) {
      channel->writeRegister(indexPush, getValue(i));
    }

    for (int i = 0; i < valuesToPush; ++i) {
      int expectedValue = getValue(i);
      int readValue = channel->readRegister(indexPop);
      registerCheck(os, expectedValue, addressPush, readValue, addressPop);
    }
  }*/
}

void crorcCleanupState(const ChannelPaths& paths)
{
  rorcCleanupState(paths);
}

void cruCleanupState(const ChannelPaths& paths)
{
  rorcCleanupState(paths);
}


} // namespace ChannelUtility
} // namespace Rorc
} // namespace AliceO2


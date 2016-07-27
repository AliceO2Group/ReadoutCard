///
/// \file ChannelUtilityImpl.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "ChannelUtilityImpl.h"
#include <iostream>
#include <functional>
#include <boost/format.hpp>

namespace AliceO2 {
namespace Rorc {
namespace ChannelUtility {

namespace b = boost;

/// Helper function for the "print FIFO" functions
void printTable(std::ostream& os, const std::string& title, const std::string& header, int size,
    std::function<void(int i)> printRow)
{
  constexpr int HEADER_INTERVAL = 32;
  auto lineFat = std::string(header.length(), '=') + '\n';
  auto lineThin = std::string(header.length(), '-') + '\n';

  os << "  " << title << "\n";
  os << lineFat << header << lineThin;

  for (int i = 0; i < size; ++i) {
    if ((i != 0) && ((i % HEADER_INTERVAL) == 0)) {
      // Add another header every x rows to make long tables more readable
      os << lineThin << header << lineThin;
    }
    printRow(i);
  }

  os << lineFat;
}

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

} // namespace ChannelUtility
} // namespace Rorc
} // namespace AliceO2
